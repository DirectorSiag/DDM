#include "jsoncommandhandler.h"

#include "../handlers/cursorcommandhandler.h"
#include "../handlers/geometrycommandhandler.h"
#include "../handlers/ownshipcommandhandler.h"
#include "../handlers/trackcommandhandler.h"
#include "../services/cpaservice.h"
#include "../services/estacionamientoservice.h"
#include "../services/obmservice.h"
#include "jsonresponsebuilder.h"
#include "commandContext.h"
#include "network/iTransport.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

JsonCommandHandler::JsonCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService, QObject* parent)
    : QObject(parent)
    , m_transport(transport)
    , m_context(context)
    , m_obmService(obmService)
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_transport);
    Q_ASSERT(m_obmService);

    m_cursorHandler = std::make_unique<CursorCommandHandler>(m_context, m_transport, m_obmService);
    m_geometryHandler = std::make_unique<GeometryCommandHandler>(m_context, m_transport, m_obmService);
    m_trackHandler = std::make_unique<TrackCommandHandler>(m_context, m_transport);
    m_ownShipHandler = std::make_unique<OwnShipCommandHandler>(m_context);
    m_cpaService = std::make_unique<CPAService>(m_context);
    m_estacionamientoService = std::make_unique<EstacionamientoService>(m_context);

    initializeCommandMap();
}

JsonCommandHandler::~JsonCommandHandler() = default;

void JsonCommandHandler::processJsonCommand(const QByteArray& jsonData)
{
    QJsonObject obj;
    if (!parseJson(jsonData, obj)) {
        return;
    }

    const QString command = obj.value(QStringLiteral("command")).toString();
    const QJsonObject args = obj.value(QStringLiteral("args")).toObject();
    routeCommand(command, args);
}

void JsonCommandHandler::refreshActiveCpaSessions()
{
    if (!m_cpaService) {
        return;
    }

    for (auto it = m_cpaSlotSessions.constBegin(); it != m_cpaSlotSessions.constEnd(); ++it) {
        const QString& sessionId = it.value();
        if (!m_cpaService->isSessionActive(sessionId)) {
            continue;
        }
        const CPAComputationResult result = m_cpaService->graphCPA(sessionId);
        if (!result.valid) {
            qWarning() << "[JsonCommandHandler] No se pudo refrescar sesion CPA" << sessionId
                       << "error:" << result.errorCode;
        }
    }
}

bool JsonCommandHandler::parseJson(const QByteArray& jsonData, QJsonObject& outObject)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        sendParseError(parseError.errorString());
        return false;
    }

    if (!doc.isObject()) {
        sendParseError(QStringLiteral("El JSON debe ser un objeto"));
        return false;
    }

    outObject = doc.object();
    return true;
}

void JsonCommandHandler::initializeCommandMap()
{
    m_commandMap[QStringLiteral("create_line")] = [this](const QJsonObject& args) {
        return m_cursorHandler->createLine(args);
    };

    m_commandMap[QStringLiteral("delete_line")] = [this](const QJsonObject& args) {
        return m_cursorHandler->deleteLine(args);
    };

    m_commandMap[QStringLiteral("list_lines")] = [this](const QJsonObject& args) {
        return m_cursorHandler->listLines(args);
    };

    m_commandMap[QStringLiteral("create_area")] = [this](const QJsonObject& args) {
        return m_geometryHandler->createArea(args);
    };

    m_commandMap[QStringLiteral("delete_area")] = [this](const QJsonObject& args) {
        return m_geometryHandler->deleteArea(args);
    };

    m_commandMap[QStringLiteral("create_circle")] = [this](const QJsonObject& args) {
        return m_geometryHandler->createCircle(args);
    };

    m_commandMap[QStringLiteral("delete_circle")] = [this](const QJsonObject& args) {
        return m_geometryHandler->deleteCircle(args);
    };

    m_commandMap[QStringLiteral("create_polygon")] = [this](const QJsonObject& args) {
        return m_geometryHandler->createPolygon(args);
    };

    m_commandMap[QStringLiteral("delete_polygon")] = [this](const QJsonObject& args) {
        return m_geometryHandler->deletePolygon(args);
    };

    m_commandMap[QStringLiteral("list_shapes")] = [this](const QJsonObject& args) {
        return m_geometryHandler->listShapes(args);
    };

    m_commandMap[QStringLiteral("create_track")] = [this](const QJsonObject& args) {
        return m_trackHandler->createTrack(args);
    };

    m_commandMap[QStringLiteral("delete_track")] = [this](const QJsonObject& args) {
        return m_trackHandler->deleteTrack(args);
    };

    m_commandMap[QStringLiteral("list_tracks")] = [this](const QJsonObject& args) {
        return m_trackHandler->listTracks(args);
    };

    m_commandMap[QStringLiteral("ownship_update")] = [this](const QJsonObject& args) {
        return m_ownShipHandler->updateOwnShip(args);
    };

    m_commandMap[QStringLiteral("cpa_start")] = [this](const QJsonObject& args) {
        return handleCpaStart(args);
    };

    m_commandMap[QStringLiteral("ppp_graph")] = [this](const QJsonObject& args) {
        return handlePppGraph(args);
    };

    m_commandMap[QStringLiteral("ppp_finish")] = [this](const QJsonObject& args) {
        return handlePppFinish(args);
    };

    m_commandMap[QStringLiteral("ppp_clear_track")] = [this](const QJsonObject& args) {
        return handlePppClearTrack(args);
    };

    m_commandMap[QStringLiteral("estacionamiento_calc")] = [this](const QJsonObject& args) {
        return handleEstacionamiento(args);
    };

    m_commandMap[QStringLiteral("estacionamiento_stop")] = [this](const QJsonObject& args) {
        return handleEstacionamientoStop(args);
    };
}

void JsonCommandHandler::routeCommand(const QString& command, const QJsonObject& args)
{
    const auto it = m_commandMap.find(command);
    if (it == m_commandMap.end()) {
        sendUnknownCommandError(command);
        return;
    }

    sendResponse(it.value()(args));
}

void JsonCommandHandler::sendResponse(const QByteArray& responseData)
{
    if (!m_transport) {
        qWarning() << "[JsonCommandHandler] Transport no disponible";
        return;
    }

    if (!m_transport->send(responseData)) {
        qWarning() << "[JsonCommandHandler] Fallo al enviar respuesta";
    }
}

void JsonCommandHandler::sendParseError(const QString& errorDetail)
{
    qWarning() << "[JsonCommandHandler] Error de parseo JSON:" << errorDetail;
    sendResponse(JsonResponseBuilder::buildErrorResponse(
        QStringLiteral("unknown"),
        QStringLiteral("INVALID_JSON"),
        QStringLiteral("Error al parsear JSON: %1").arg(errorDetail)
    ));
}

void JsonCommandHandler::sendUnknownCommandError(const QString& command)
{
    qWarning() << "[JsonCommandHandler] Comando no reconocido:" << command;
    sendResponse(JsonResponseBuilder::buildErrorResponse(
        command,
        QStringLiteral("UNKNOWN_COMMAND"),
        QStringLiteral("Comando no reconocido: %1").arg(command)
    ));
}

bool JsonCommandHandler::parseTrackRefValue(const QJsonValue& value, CPATrackRef& outRef, QString& errorReason) const
{
    if (value.isDouble()) {
        const int id = value.toInt(-1);
        if (id < 0) {
            errorReason = QStringLiteral("must_be_integer_ge_0");
            return false;
        }

        outRef.isOwnShip = false;
        outRef.trackId = id;
        return true;
    }

    if (value.isString()) {
        const QString token = value.toString().trimmed().toLower();
        if (token == QStringLiteral("own_ship")
            || token == QStringLiteral("ownship")
            || token == QStringLiteral("os")
            || token == QStringLiteral("own")) {
            outRef.isOwnShip = true;
            outRef.trackId = -1;
            return true;
        }

        bool ok = false;
        const int id = token.toInt(&ok);
        if (ok && id >= 0) {
            outRef.isOwnShip = false;
            outRef.trackId = id;
            return true;
        }

        errorReason = QStringLiteral("invalid_track_reference");
        return false;
    }

    errorReason = QStringLiteral("unsupported_type");
    return false;
}

bool JsonCommandHandler::parseTrackPair(const QJsonObject& args,
                                        CPATrackRef& trackA,
                                        CPATrackRef& trackB,
                                        QString& errorField,
                                        QString& errorReason) const
{
    const QJsonValue trackAValue = args.contains(QStringLiteral("track_a")) ? args.value(QStringLiteral("track_a")) : args.value(QStringLiteral("track-a"));
    const QJsonValue trackBValue = args.contains(QStringLiteral("track_b")) ? args.value(QStringLiteral("track_b")) : args.value(QStringLiteral("track-b"));

    if (trackAValue.isUndefined()) {
        errorField = QStringLiteral("track_a");
        errorReason = QStringLiteral("required");
        return false;
    }

    if (trackBValue.isUndefined()) {
        errorField = QStringLiteral("track_b");
        errorReason = QStringLiteral("required");
        return false;
    }

    if (!parseTrackRefValue(trackAValue, trackA, errorReason)) {
        errorField = QStringLiteral("track_a");
        return false;
    }

    if (!parseTrackRefValue(trackBValue, trackB, errorReason)) {
        errorField = QStringLiteral("track_b");
        return false;
    }

    if (trackB.isOwnShip) {
        errorField = QStringLiteral("track_b");
        errorReason = QStringLiteral("own_ship_not_allowed_in_track_b");
        return false;
    }

    return true;
}

QByteArray JsonCommandHandler::handleCpaStart(const QJsonObject& args)
{
    const int calcIndex = args.value(QStringLiteral("index")).toInt(-1);
    if (calcIndex < 0) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("cpa_start"), QStringLiteral("index"), QString(), QStringLiteral("required, must be >= 0"));
    }

    CPATrackRef trackA;
    CPATrackRef trackB;
    QString errorField;
    QString errorReason;

    if (!parseTrackPair(args, trackA, trackB, errorField, errorReason)) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("cpa_start"), errorField, QString(), errorReason);
    }

    const CPAComputationResult result = m_cpaService->startCPA(trackA, trackB);
    if (!result.valid) {
        return JsonResponseBuilder::buildErrorResponse(QStringLiteral("cpa_start"), result.errorCode, result.errorMessage);
    }

    // Mantener la relacion slot->session para comandos ppp_graph/finish/clear_track.
    m_cpaSlotSessions[calcIndex] = result.sessionId;

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("index")] = calcIndex;
    responseArgs[QStringLiteral("id")] = result.sessionId;
    responseArgs[QStringLiteral("track_a")] = trackA.isOwnShip ? QJsonValue(QStringLiteral("own_ship")) : QJsonValue(trackA.trackId);
    responseArgs[QStringLiteral("track_b")] = trackB.isOwnShip ? QJsonValue(QStringLiteral("own_ship")) : QJsonValue(trackB.trackId);
    responseArgs[QStringLiteral("tcpa_sec")] = result.tcpaSeconds;
    responseArgs[QStringLiteral("dcpa_dm")] = result.dcpaDm;
    responseArgs[QStringLiteral("cpa_mid_x")] = result.cpaMidX;
    responseArgs[QStringLiteral("cpa_mid_y")] = result.cpaMidY;
    responseArgs[QStringLiteral("status")] = QStringLiteral("active");

    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("cpa_start"), responseArgs);
}

QByteArray JsonCommandHandler::handlePppGraph(const QJsonObject& args)
{
    const int calcIndex = args.value(QStringLiteral("calc_index")).toInt(-1);
    if (calcIndex < 0) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("ppp_graph"), QStringLiteral("calc_index"), QString(), QStringLiteral("required, must be >= 0"));
    }

    const auto sessionIt = m_cpaSlotSessions.find(calcIndex);
    if (sessionIt == m_cpaSlotSessions.end()) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("ppp_graph"),
            QStringLiteral("SESSION_NOT_FOUND"),
            QStringLiteral("No existe sesion CPA para calc_index %1. Ejecute cpa_start primero.").arg(calcIndex)
        );
    }

    const QString sessionId = sessionIt.value();

    const CPAComputationResult result = m_cpaService->graphCPA(sessionId);
    if (!result.valid) {
        if (result.errorCode == QStringLiteral("cpa_expired")) {
            QJsonObject responseArgs;
            responseArgs[QStringLiteral("calc_index")] = calcIndex;
            responseArgs[QStringLiteral("id")] = sessionId;
            responseArgs[QStringLiteral("status")] = QStringLiteral("cpa_expired");
            responseArgs[QStringLiteral("message")] = result.errorMessage;
            return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("ppp_graph"), responseArgs);
        }
        return JsonResponseBuilder::buildErrorResponse(QStringLiteral("ppp_graph"), result.errorCode, result.errorMessage);
    }

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("calc_index")] = calcIndex;
    responseArgs[QStringLiteral("id")] = sessionId;
    responseArgs[QStringLiteral("tcpa_sec")] = result.tcpaSeconds;
    responseArgs[QStringLiteral("dcpa_dm")] = result.dcpaDm;
    responseArgs[QStringLiteral("cpa_mid_x")] = result.cpaMidX;
    responseArgs[QStringLiteral("cpa_mid_y")] = result.cpaMidY;
    responseArgs[QStringLiteral("symbol")] = QStringLiteral("C3F07");
    responseArgs[QStringLiteral("main_symbol_byte")] = 0x26;

    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("ppp_graph"), responseArgs);
}

QByteArray JsonCommandHandler::handlePppFinish(const QJsonObject& args)
{
    const int calcIndex = args.value(QStringLiteral("calc_index")).toInt(-1);
    if (calcIndex < 0) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("ppp_finish"), QStringLiteral("calc_index"), QString(), QStringLiteral("required, must be >= 0"));
    }

    const auto sessionIt = m_cpaSlotSessions.find(calcIndex);
    if (sessionIt == m_cpaSlotSessions.end()) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("ppp_finish"),
            QStringLiteral("SESSION_NOT_FOUND"),
            QStringLiteral("No existe sesion CPA para calc_index %1. Ejecute cpa_start primero.").arg(calcIndex)
        );
    }

    const QString sessionId = sessionIt.value();

    if (!m_cpaService->finishCPA(sessionId)) {
        return JsonResponseBuilder::buildErrorResponse(QStringLiteral("ppp_finish"), QStringLiteral("SESSION_NOT_FOUND"), QStringLiteral("No existe sesion CPA activa para calc_index %1").arg(calcIndex));
    }

    m_cpaSlotSessions.remove(calcIndex);

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("calc_index")] = calcIndex;
    responseArgs[QStringLiteral("id")] = sessionId;
    responseArgs[QStringLiteral("status")] = QStringLiteral("finished");
    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("ppp_finish"), responseArgs);
}

QByteArray JsonCommandHandler::handlePppClearTrack(const QJsonObject& args)
{
    const int calcIndex = args.value(QStringLiteral("calc_index")).toInt(-1);
    if (calcIndex < 0) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("ppp_clear_track"), QStringLiteral("calc_index"), QString(), QStringLiteral("required, must be >= 0"));
    }

    const QString field = args.value(QStringLiteral("field")).toString();
    if (field.isEmpty() || (field != QStringLiteral("track_a") && field != QStringLiteral("track_b"))) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("ppp_clear_track"), QStringLiteral("field"), field, QStringLiteral("must be 'track_a' or 'track_b'"));
    }

    const auto sessionIt = m_cpaSlotSessions.find(calcIndex);
    if (sessionIt == m_cpaSlotSessions.end()) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("ppp_clear_track"),
            QStringLiteral("SESSION_NOT_FOUND"),
            QStringLiteral("No existe sesion CPA para calc_index %1. Ejecute cpa_start primero.").arg(calcIndex)
        );
    }

    const QString sessionId = sessionIt.value();

    if (!m_cpaService->finishCPA(sessionId)) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("ppp_clear_track"),
            QStringLiteral("SESSION_NOT_FOUND"),
            QStringLiteral("No existe sesion CPA activa para calc_index %1").arg(calcIndex)
        );
    }

    m_cpaSlotSessions.remove(calcIndex);

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("calc_index")] = calcIndex;
    responseArgs[QStringLiteral("id")] = sessionId;
    responseArgs[QStringLiteral("field")] = field;
    responseArgs[QStringLiteral("removed_sessions")] = 1;
    responseArgs[QStringLiteral("removed_markers")] = 1;
    responseArgs[QStringLiteral("status")] = QStringLiteral("cleared");
    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("ppp_clear_track"), responseArgs);
}

QByteArray JsonCommandHandler::handleEstacionamiento(const QJsonObject& args)
{
    const int slotIndex = args.value(QStringLiteral("index")).toInt(-1);
    if (slotIndex < 1 || slotIndex > 10) {
        return JsonResponseBuilder::buildValidationErrorResponse(
            QStringLiteral("estacionamiento_calc"),
            QStringLiteral("index"),
            QString::number(slotIndex),
            QStringLiteral("required, must be between 1 and 10")
        );
    }

    QMap<QString, QString> options;

    auto valueToString = [](const QJsonValue& value, QString& outText) -> bool {
        if (value.isDouble()) {
            outText = QString::number(value.toDouble(), 'g', 16);
            return true;
        }
        if (value.isString()) {
            outText = value.toString().trimmed();
            return !outText.isEmpty();
        }
        return false;
    };

    const QJsonValue trackAValue = args.contains(QStringLiteral("track_a")) ? args.value(QStringLiteral("track_a")) : args.value(QStringLiteral("track-a"));
    if (!trackAValue.isUndefined()) {
        CPATrackRef trackARef;
        QString errorReason;
        if (!parseTrackRefValue(trackAValue, trackARef, errorReason)) {
            return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("track_a"), QString(), errorReason);
        }
        options.insert(QStringLiteral("track-a"), trackARef.isOwnShip ? QStringLiteral("0") : QString::number(trackARef.trackId));
    }

    const QJsonValue trackBValue = args.contains(QStringLiteral("track_b")) ? args.value(QStringLiteral("track_b")) : args.value(QStringLiteral("track-b"));
    if (trackBValue.isUndefined()) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("track_b"), QString(), QStringLiteral("required"));
    }
    CPATrackRef trackBRef;
    QString trackBError;
    if (!parseTrackRefValue(trackBValue, trackBRef, trackBError)) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("track_b"), QString(), trackBError);
    }
    options.insert(QStringLiteral("track-b"), trackBRef.isOwnShip ? QStringLiteral("0") : QString::number(trackBRef.trackId));

    QString optionValue;
    const QJsonValue azValue = args.value(QStringLiteral("az"));
    if (!valueToString(azValue, optionValue)) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("az"), QString(), QStringLiteral("required numeric"));
    }
    options.insert(QStringLiteral("az"), optionValue);

    const QJsonValue distValue = args.value(QStringLiteral("d"));
    if (!valueToString(distValue, optionValue)) {
        return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("d"), QString(), QStringLiteral("required numeric"));
    }
    options.insert(QStringLiteral("d"), optionValue);

    const QJsonValue vdValue = args.value(QStringLiteral("vd"));
    if (!vdValue.isUndefined()) {
        if (!valueToString(vdValue, optionValue)) {
            return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("vd"), QString(), QStringLiteral("must be numeric"));
        }
        options.insert(QStringLiteral("vd"), optionValue);
    }

    const QJsonValue duValue = args.value(QStringLiteral("du"));
    if (!duValue.isUndefined()) {
        if (!valueToString(duValue, optionValue)) {
            return JsonResponseBuilder::buildValidationErrorResponse(QStringLiteral("estacionamiento_calc"), QStringLiteral("du"), QString(), QStringLiteral("must be numeric"));
        }
        options.insert(QStringLiteral("du"), optionValue);
    }

    const EstacionamientoService::CalculationResult calcResult = m_estacionamientoService->calculateFromOptions(options);
    if (!calcResult.success) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("estacionamiento_calc"),
            QStringLiteral("VALIDATION_ERROR"),
            calcResult.errorMessage
        );
    }

    CommandContext::StationingSession session;
    session.slotIndex = slotIndex;
    session.trackAId = calcResult.trackAId;
    session.trackBId = calcResult.trackBId;
    session.azimuth = calcResult.azimuthDeg;
    session.distance = calcResult.distanceDm;
    session.modalidad = calcResult.modalidad;
    session.valorModalidad = calcResult.modalidadValue;
    session.rumboDeg = calcResult.rumboDeg;
    session.tiempoManiobra = calcResult.timeHours;
    session.posicionEstacionX = calcResult.stationPosXDm;
    session.posicionEstacionY = calcResult.stationPosYDm;

    if (!m_context->upsertStationingSession(session)) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("estacionamiento_calc"),
            QStringLiteral("INVALID_SLOT"),
            QStringLiteral("No se pudo persistir la sesion de estacionamiento para index %1").arg(slotIndex)
        );
    }

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("index")] = slotIndex;
    responseArgs[QStringLiteral("track_a")] = calcResult.trackAId;
    responseArgs[QStringLiteral("track_b")] = calcResult.trackBId;
    responseArgs[QStringLiteral("rumbo")] = calcResult.rumboDeg;
    responseArgs[QStringLiteral("tiempo_horas")] = calcResult.timeHours;
    responseArgs[QStringLiteral("tiempo_hms")] = calcResult.timeHms;
    responseArgs[QStringLiteral("station_x_dm")] = calcResult.stationPosXDm;
    responseArgs[QStringLiteral("station_y_dm")] = calcResult.stationPosYDm;

    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("estacionamiento_calc"), responseArgs);
}

QByteArray JsonCommandHandler::handleEstacionamientoStop(const QJsonObject& args)
{
    const int slotIndex = args.value(QStringLiteral("index")).toInt(-1);
    if (slotIndex < 1 || slotIndex > 10) {
        return JsonResponseBuilder::buildValidationErrorResponse(
            QStringLiteral("estacionamiento_stop"),
            QStringLiteral("index"),
            QString::number(slotIndex),
            QStringLiteral("required, must be between 1 and 10")
        );
    }

    if (!m_context->removeStationingSession(slotIndex)) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("estacionamiento_stop"),
            QStringLiteral("SESSION_NOT_FOUND"),
            QStringLiteral("No existe sesion de estacionamiento activa para index %1").arg(slotIndex)
        );
    }

    QJsonObject responseArgs;
    responseArgs[QStringLiteral("index")] = slotIndex;
    responseArgs[QStringLiteral("status")] = QStringLiteral("stopped");
    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("estacionamiento_stop"), responseArgs);
}
