#include "trackcommandhandler.h"
#include "../json/jsonresponsebuilder.h"
#include "../services/trackservice.h"
#include "commandContext.h"
#include "network/iTransport.h"
#include "entities/track.h"
#include "enums/enums.h"
#include <QJsonArray>
#include <QDebug>

namespace {

bool parseTrackTypeValue(const QJsonValue& value, TrackData::Type& outType)
{
    if (value.isString()) {
        const QString raw = value.toString().trimmed();
        if (TrackData::tryParseType(raw, outType)) {
            return true;
        }
        if (TrackData::tryParseTypeBits(raw, outType)) {
            return true;
        }
        return false;
    }

    if (value.isDouble()) {
        const int numeric = value.toInt(-1);
        if (numeric < 1 || numeric > 8) {
            return false;
        }
        return TrackData::tryParseTypeBits(QStringLiteral("%1").arg(numeric, 4, 2, QLatin1Char('0')), outType);
    }

    return false;
}

} // namespace

TrackCommandHandler::TrackCommandHandler(CommandContext* context, ITransport* transport)
    : m_context(context), m_transport(transport), m_trackService(std::make_unique<TrackService>(context))
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_transport);
}

QByteArray TrackCommandHandler::createTrack(const QJsonObject& args)
{
    // Validar coordenadas
    if (!args.contains("x") || !args.contains("y")) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_track", "x/y", "", "required");
    }

    TrackCreateRequest request;
    request.x = args.value("x").toDouble();
    request.y = args.value("y").toDouble();

    if (args.contains("type")) {
        TrackData::Type parsedType = TrackData::SPC;
        if (!parseTrackTypeValue(args.value("type"), parsedType)) {
            return JsonResponseBuilder::buildValidationErrorResponse(
                "create_track", "type", args.value("type").toVariant().toString(),
                "SPC|LINCO|ASW|OPS|HECO|APC|AAW|EW o bits 0001..1000"
            );
        }
        request.type = parsedType;
    }

    const QJsonValue environmentValue =
        args.contains("creation_environment") ? args.value("creation_environment") :
        args.contains("environment") ? args.value("environment") :
        args.value("ambiente");

    if (!environmentValue.isUndefined()) {
        TrackData::Type parsedEnvironment = TrackData::SPC;
        if (!parseTrackTypeValue(environmentValue, parsedEnvironment)) {
            return JsonResponseBuilder::buildValidationErrorResponse(
                "create_track", "creation_environment", environmentValue.toVariant().toString(),
                "SPC|LINCO|ASW|OPS|HECO|APC|AAW|EW o bits 0001..1000"
            );
        }
        request.creationEnvironment = parsedEnvironment;
    }

    if (args.contains("info")) {
        request.info = args.value("info").toString();
    }

    TrackOperationResult result = m_trackService->createTrack(request);
    if (!result.success) {
        qWarning() << "[TrackCommandHandler] Error al crear track:" << result.message;
        return JsonResponseBuilder::buildErrorResponse("create_track", result.errorCode, result.message);
    }

    return buildCreateTrackResponse(result.trackId);
}

QByteArray TrackCommandHandler::deleteTrack(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_track", "id", "", "required");
    }
    int id = args.value("id").toInt(-1);
    if (id < 0) return JsonResponseBuilder::buildValidationErrorResponse("delete_track", "id", QString::number(id), ">=0");

    TrackOperationResult result = m_trackService->deleteTrackById(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_track", result.errorCode, result.message);
    }

    return buildDeleteTrackResponse(id);
}

QByteArray TrackCommandHandler::listTracks(const QJsonObject& /*args*/)
{
    return buildListTracksResponse();
}

QByteArray TrackCommandHandler::buildCreateTrackResponse(int createdId)
{
    QJsonObject args;
    args["created_id"] = QString::number(createdId);
    args["tracks"] = m_trackService->serializeTracks();
    return JsonResponseBuilder::buildSuccessResponse("create_track", args);
}

QByteArray TrackCommandHandler::buildDeleteTrackResponse(int deletedId)
{
    QJsonObject args;
    args["deleted_id"] = deletedId;
    args["tracks"] = m_trackService->serializeTracks();
    return JsonResponseBuilder::buildSuccessResponse("delete_track", args);
}

QByteArray TrackCommandHandler::buildListTracksResponse()
{
    QJsonObject args;
    args["tracks"] = m_trackService->serializeTracks();
    return JsonResponseBuilder::buildSuccessResponse("list_tracks", args);
}