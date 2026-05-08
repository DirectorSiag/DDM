#include "ownshipservice.h"

#include "commandContext.h"
#include "entities/track.h"
#include "trackpppservice.h"

#include <QtMath>
#include <cmath>

OwnShipService::OwnShipService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

double OwnShipService::normalize360(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) {
        deg += 360.0;
    }
    return deg;
}

void OwnShipService::syncOwnShipVirtualTrack()
{
    auto& own = m_context->ownShip;
    Track* ownTrack = m_context->findTrackById(0);

    if (!ownTrack) {
        ownTrack = &m_context->emplaceTrackFront(
            0,
            TrackData::SPC,
            TrackData::Pending,
            TrackData::Auto,
            0.0f,
            0.0f,
            own.speedKnots,
            own.courseDeg,
            TrackData::SPC
        );
    }

    ownTrack->setX(0.0f);
    ownTrack->setY(0.0f);
    ownTrack->setCourseDeg(own.courseDeg);
    ownTrack->setSpeedKnots(own.speedKnots);
}

OwnShipOperationResult OwnShipService::updateFromJson(const QJsonObject& args)
{
    if (!args.contains("course_deg") || !args.contains("speed_knots")) {
        return {false, "MISSING_FIELDS", "Faltan campos requeridos: course_deg y speed_knots"};
    }

    const QJsonValue courseValue = args.value("course_deg");
    const QJsonValue speedValue = args.value("speed_knots");
    if (!courseValue.isDouble() || !speedValue.isDouble()) {
        return {false, "INVALID_FIELDS", "course_deg y speed_knots deben ser numericos"};
    }

    const double speedKnots = speedValue.toDouble();
    if (!std::isfinite(speedKnots) || speedKnots < 0.0) {
        return {false, "INVALID_SPEED", "speed_knots debe ser >= 0"};
    }

    auto& own = m_context->ownShip;
    own.xDm = 0.0;
    own.yDm = 0.0;
    own.courseDeg = normalize360(courseValue.toDouble());
    own.speedKnots = speedKnots;

    if (args.contains("latitude_deg") && args.value("latitude_deg").isDouble()) {
        const double lat = args.value("latitude_deg").toDouble();
        if (lat < -90.0 || lat > 90.0) {
            return {false, "INVALID_LATITUDE", "latitude_deg fuera de rango [-90,90]"};
        }
        own.latitudeDeg = lat;
    }

    if (args.contains("longitude_deg") && args.value("longitude_deg").isDouble()) {
        const double lon = args.value("longitude_deg").toDouble();
        if (lon < -180.0 || lon > 180.0) {
            return {false, "INVALID_LONGITUDE", "longitude_deg fuera de rango [-180,180]"};
        }
        own.longitudeDeg = lon;
    }

    if (args.contains("time_utc") && args.value("time_utc").isString()) {
        own.timeUtc = args.value("time_utc").toString();
    }

    if (args.contains("date_utc") && args.value("date_utc").isString()) {
        own.dateUtc = args.value("date_utc").toString();
    }

    if (args.contains("source") && args.value("source").isString()) {
        own.source = args.value("source").toString();
    }

    own.valid = true;
    syncOwnShipVirtualTrack();

    // Etapa actual: tracks estaticos.
    // Por eso el PPP de SITREP se recalcula una sola vez cuando cambia OwnShip.
    // Si en el futuro los tracks pasan a ser dinamicos, este recalc tambien debe
    // dispararse desde el punto de actualizacion cinematica (por ejemplo updateTracks()).
    TrackPppService(m_context).recalculateAllTracksAgainstOwnShip();

    return {true, QString(), QStringLiteral("OwnShip actualizado")};
}

OwnShipOperationResult OwnShipService::setFromCli(double courseDeg,
                                                  double speedKnots,
                                                  const QString& source)
{
    if (!std::isfinite(courseDeg) || !std::isfinite(speedKnots)) {
        return {false, "INVALID_VALUES", "Valores no finitos"};
    }

    if (speedKnots < 0.0) {
        return {false, "INVALID_SPEED", "La velocidad debe ser >= 0"};
    }

    auto& own = m_context->ownShip;
    own.xDm = 0.0;
    own.yDm = 0.0;
    own.courseDeg = normalize360(courseDeg);
    own.speedKnots = speedKnots;
    own.source = source.trimmed().isEmpty() ? QStringLiteral("CLI") : source;
    own.valid = true;
    syncOwnShipVirtualTrack();

    // Etapa actual: tracks estaticos.
    // Por eso el PPP de SITREP se recalcula una sola vez cuando cambia OwnShip.
    // Si en el futuro los tracks pasan a ser dinamicos, este recalc tambien debe
    // dispararse desde el punto de actualizacion cinematica (por ejemplo updateTracks()).
    TrackPppService(m_context).recalculateAllTracksAgainstOwnShip();

    return {true, QString(), QStringLiteral("OwnShip actualizado desde consola")};
}

QJsonObject OwnShipService::serializeOwnShip() const
{
    const auto& own = m_context->ownShip;
    QJsonObject ownObj;
    ownObj["valid"] = own.valid;
    ownObj["x_dm"] = own.xDm;
    ownObj["y_dm"] = own.yDm;
    ownObj["latitude_deg"] = own.latitudeDeg;
    ownObj["longitude_deg"] = own.longitudeDeg;
    ownObj["course_deg"] = own.courseDeg;
    ownObj["speed_knots"] = own.speedKnots;
    ownObj["time_utc"] = own.timeUtc;
    ownObj["date_utc"] = own.dateUtc;
    ownObj["source"] = own.source;
    return ownObj;
}

QString OwnShipService::formatOwnShip() const
{
    const auto& own = m_context->ownShip;
    return QStringLiteral("OwnShip{valid=%1, pos=(%2,%3)DM, lat=%4, lon=%5, crs=%6deg, spd=%7kt, time=%8, date=%9, source=%10}")
        .arg(own.valid ? QStringLiteral("true") : QStringLiteral("false"))
        .arg(own.xDm, 0, 'f', 3)
        .arg(own.yDm, 0, 'f', 3)
        .arg(own.latitudeDeg, 0, 'f', 6)
        .arg(own.longitudeDeg, 0, 'f', 6)
        .arg(own.courseDeg, 0, 'f', 2)
        .arg(own.speedKnots, 0, 'f', 2)
        .arg(own.timeUtc)
        .arg(own.dateUtc)
        .arg(own.source);
}
