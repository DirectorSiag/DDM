#include "estacionamientoservice.h"

#include "estacionamientocalculator.h"
#include "commandContext.h"
#include "trackservice.h"

#include <QMap>
#include <QSet>
#include <QTextStream>
#include <QtMath>

#include <cmath>

namespace {
QString normalizeKey(const QString& key)
{
    QString normalized = key.trimmed();
    while (normalized.startsWith('-')) {
        normalized.remove(0, 1);
    }
    return normalized.toLower();
}

bool parseDoubleText(const QString& text, double& out)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

bool parseIntegerText(const QString& text, int& out)
{
    bool ok = false;
    const int value = text.toInt(&ok, 10);
    if (!ok) {
        return false;
    }
    out = value;
    return true;
}
}

EstacionamientoService::EstacionamientoService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

bool EstacionamientoService::parseOptions(const QStringList& args,
                                          QMap<QString, QString>& options,
                                          QString& error)
{
    for (const QString& token : args) {
        const int eqPos = token.indexOf('=');
        if (eqPos <= 0) {
            error = QStringLiteral("Token invalido: %1. Use formato --clave=valor").arg(token);
            return false;
        }

        const QString key = normalizeKey(token.left(eqPos));
        const QString value = token.mid(eqPos + 1).trimmed();
        if (key.isEmpty() || value.isEmpty()) {
            error = QStringLiteral("Token invalido: %1. Use formato --clave=valor").arg(token);
            return false;
        }

        options.insert(key, value);
    }

    return true;
}

bool EstacionamientoService::validateAllowedOptions(const QMap<QString, QString>& options,
                                                    QString& error)
{
    static const QSet<QString> allowedKeys = {
        QStringLiteral("track-a"),
        QStringLiteral("track-b"),
        QStringLiteral("az"),
        QStringLiteral("d"),
        QStringLiteral("vd"),
        QStringLiteral("du")
    };

    for (auto it = options.begin(); it != options.end(); ++it) {
        if (!allowedKeys.contains(it.key())) {
            error = QStringLiteral("Parametro no permitido para estacionamiento: --%1").arg(it.key());
            return false;
        }
    }
    return true;
}

bool EstacionamientoService::parseDoubleField(const QMap<QString, QString>& options,
                                              const QString& key,
                                              bool required,
                                              double& value,
                                              QString& error)
{
    const auto it = options.find(key);
    if (it == options.end()) {
        if (required) {
            error = QStringLiteral("Falta campo requerido: --%1").arg(key);
            return false;
        }
        return true;
    }

    if (!parseDoubleText(it.value(), value)) {
        error = QStringLiteral("Campo numerico invalido en --%1").arg(key);
        return false;
    }

    return true;
}

bool EstacionamientoService::parseIntegerField(const QMap<QString, QString>& options,
                                               const QString& key,
                                               bool required,
                                               int& value,
                                               QString& error)
{
    const auto it = options.find(key);
    if (it == options.end()) {
        if (required) {
            error = QStringLiteral("Falta campo requerido: --%1").arg(key);
            return false;
        }
        return true;
    }

    if (!parseIntegerText(it.value(), value)) {
        error = QStringLiteral("Campo entero invalido en --%1").arg(key);
        return false;
    }
    return true;
}

bool EstacionamientoService::loadAndValidate(const QMap<QString, QString>& options,
                                             CliInput& input,
                                             QString& error)
{
    if (!validateAllowedOptions(options, error)) {
        return false;
    }

    if (!parseIntegerField(options, QStringLiteral("track-a"), false, input.trackAId, error)
        || !parseIntegerField(options, QStringLiteral("track-b"), true, input.trackBId, error)) {
        return false;
    }

    if (!parseDoubleField(options, QStringLiteral("az"), true, input.azRelativeDeg, error)
        || !parseDoubleField(options, QStringLiteral("d"), true, input.distanceDm, error)) {
        return false;
    }

    const auto itVd = options.find(QStringLiteral("vd"));
    const auto itDu = options.find(QStringLiteral("du"));
    input.hasVd = (itVd != options.end());
    input.hasDu = (itDu != options.end());

    if (input.hasVd == input.hasDu) {
        error = QStringLiteral("Debe especificar exactamente una modalidad: --vd o --du");
        return false;
    }

    if (input.distanceDm <= 0.0) {
        error = QStringLiteral("La distancia --d debe ser > 0");
        return false;
    }

    if (input.trackAId < 0 || input.trackBId < 0) {
        error = QStringLiteral("--track-a y --track-b deben ser IDs enteros >= 0");
        return false;
    }

    // Default de negocio: si se omite --track-a, usar 0000 (OwnShip).
    if (!options.contains(QStringLiteral("track-a"))) {
        input.trackAId = 0;
    }

    if (input.trackBId == 0) {
        error = QStringLiteral("Error: El TRACK-B no puede ser el buque propio (0000). Debe ser un track externo");
        return false;
    }

    if (input.hasVd) {
        if (!parseDoubleText(itVd.value(), input.vdKnots) || input.vdKnots <= 0.0) {
            error = QStringLiteral("--vd debe ser un valor numerico > 0");
            return false;
        }
    }

    if (input.hasDu) {
        if (!parseDoubleText(itDu.value(), input.duHours) || input.duHours <= 0.0) {
            error = QStringLiteral("--du invalido. Use horas decimales > 0");
            return false;
        }
    }

    return true;
}

QString EstacionamientoService::formatDurationHms(double hours)
{
    if (!std::isfinite(hours) || hours < 0.0) {
        return QStringLiteral("00:00:00");
    }

    qint64 totalSeconds = static_cast<qint64>(std::llround(hours * 3600.0));
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }

    const qint64 hh = totalSeconds / 3600;
    const qint64 mm = (totalSeconds % 3600) / 60;
    const qint64 ss = totalSeconds % 60;

    return QStringLiteral("%1:%2:%3")
        .arg(hh, 2, 10, QLatin1Char('0'))
        .arg(mm, 2, 10, QLatin1Char('0'))
        .arg(ss, 2, 10, QLatin1Char('0'));
}

double EstacionamientoService::knotsToDmPerHour(double knots)
{
    return knots / kDmToNm;
}

EstacionamientoService::OperationResult EstacionamientoService::executeFromCliArgs(const QStringList& args) const
{
    QMap<QString, QString> options;
    QString parseError;
    if (!parseOptions(args, options, parseError)) {
        return {false, parseError};
    }

    const CalculationResult calcResult = calculateFromOptions(options);
    if (!calcResult.success) {
        return {false, calcResult.errorMessage};
    }

    const int roundedHeading = static_cast<int>(std::lround(calcResult.rumboDeg)) % 360;
    QString output;
    QTextStream stream(&output);
    stream << "TRACK-A: " << calcResult.trackAId << " / TRACK-B: " << calcResult.trackBId << "\n";
    stream << "RUMBO: " << roundedHeading << " / TIEMPO: H " << calcResult.timeHms;

    return {true, output};
}

EstacionamientoService::CalculationResult EstacionamientoService::calculateFromOptions(const QMap<QString, QString>& options) const
{
    CalculationResult out;

    if (!m_context) {
        out.errorMessage = QStringLiteral("Contexto no inicializado para EstacionamientoService");
        return out;
    }

    CliInput input;
    QString validationError;
    if (!loadAndValidate(options, input, validationError)) {
        out.errorMessage = validationError;
        return out;
    }

    TrackService trackService(m_context);

    auto resolveKinematicState = [&](int trackId,
                                     const QString& label,
                                     EstacionamientoCalculator::KinematicState& outState,
                                     QString& resolutionError) -> bool {
        if (trackId == 0) {
            const auto& own = m_context->ownShip;
            if (!own.valid
                || !std::isfinite(own.courseDeg)
                || !std::isfinite(own.speedKnots)
                || own.speedKnots < 0.0) {
                resolutionError = QStringLiteral("OwnShip no inicializado. Ejecute 'ownship set <course_deg> <speed_knots> [source]' antes de usar track 0000");
                return false;
            }

            outState = {
                0.0,
                0.0,
                knotsToDmPerHour(own.speedKnots),
                own.courseDeg,
                true
            };
            return true;
        }

        Track* track = trackService.findTrackById(trackId);
        if (!track) {
            resolutionError = QStringLiteral("%1 no encontrado: %2").arg(label).arg(trackId);
            return false;
        }

        outState = {
            track->getX(),
            track->getY(),
            knotsToDmPerHour(track->getSpeedKnots()),
            track->getCourseDeg(),
            true
        };
        return true;
    };

    EstacionamientoCalculator::KinematicState stateA;
    EstacionamientoCalculator::KinematicState stateB;
    QString resolutionError;
    if (!resolveKinematicState(input.trackAId, QStringLiteral("Track A"), stateA, resolutionError)) {
        out.errorMessage = resolutionError;
        return out;
    }
    if (!resolveKinematicState(input.trackBId, QStringLiteral("Track B"), stateB, resolutionError)) {
        out.errorMessage = resolutionError;
        return out;
    }

    EstacionamientoCalculator::Input calcInput;
    calcInput.trackA = stateA;
    calcInput.trackB = stateB;
    calcInput.azRelativeDeg = input.azRelativeDeg;
    calcInput.distanceDm = input.distanceDm;
    calcInput.useSpeedMode = input.hasVd;
    calcInput.vdDmPerHour = knotsToDmPerHour(input.vdKnots);
    calcInput.duHours = input.duHours;

    const EstacionamientoCalculator::Result result = EstacionamientoCalculator::compute(calcInput);
    if (result.status != EstacionamientoCalculator::Result::Valid) {
        out.errorMessage = QStringLiteral("No se pudo resolver Estacionamiento: %1").arg(result.reason);
        return out;
    }

    out.success = true;
    out.trackAId = input.trackAId;
    out.trackBId = input.trackBId;
    out.azimuthDeg = input.azRelativeDeg;
    out.distanceDm = input.distanceDm;
    out.modalidad = input.hasVd ? QStringLiteral("VD") : QStringLiteral("DU");
    out.modalidadValue = input.hasVd ? input.vdKnots : input.duHours;
    out.rumboDeg = result.rumboDeg;
    out.timeHours = result.timeHours;
    out.timeHms = formatDurationHms(result.timeHours);

    // Posicion de estacionamiento actual (offset sobre TRACK-B segun azimut relativo).
    const double stationAzAbsDeg = std::fmod(stateB.courseDeg + input.azRelativeDeg, 360.0);
    const double normalizedAzAbsDeg = (stationAzAbsDeg < 0.0) ? (stationAzAbsDeg + 360.0) : stationAzAbsDeg;
    const double stationAzAbsRad = qDegreesToRadians(normalizedAzAbsDeg);
    out.stationPosXDm = stateB.xDm + (input.distanceDm * std::sin(stationAzAbsRad));
    out.stationPosYDm = stateB.yDm + (input.distanceDm * std::cos(stationAzAbsRad));

    return out;
}
