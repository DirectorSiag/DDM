#include "jsonserializer.h"
#include "entities/cursorEntity.h"
#include "enums/enums.h"

#include <QJsonDocument>
#include <QMetaEnum>

QJsonObject JsonSerializer::serializeLine(const CursorEntity& cursor)
{
    QJsonObject lineObj;
    lineObj["id"] = QString("LINE_%1").arg(cursor.getCursorId());
    lineObj["x"] = static_cast<double>(cursor.getCoordinates().first);
    lineObj["y"] = static_cast<double>(cursor.getCoordinates().second);
    lineObj["azimut"] = static_cast<double>(cursor.getCursorAngle());
    lineObj["length"] = static_cast<double>(cursor.getCursorLength());
    lineObj["type"] = cursor.getLineType();
    lineObj["active"] = cursor.isActive();
    
    return lineObj;
}

QJsonObject JsonSerializer::serializeTrackForReplication(const Track& track)
{
    QJsonObject obj;
    obj["guid"]                = QString::fromStdString(track.getGuid());
    obj["type"]                = TrackData::toQString(track.getType());
    obj["identity"]            = TrackData::toQString(track.getIdentity());
    obj["mode"]                = TrackData::toQString(track.getTrackMode());
    obj["creation_environment"]= TrackData::toQString(track.getCreationEnvironment());
    obj["x"]                   = static_cast<double>(track.getX());
    obj["y"]                   = static_cast<double>(track.getY());
    obj["speed_dm_h"]          = track.getVelocidadDmPerHour();
    obj["course_deg"]          = track.getCursoInt();
    obj["info"]                = track.getInformacionAmpliatoria();
    obj["asignacion_fc"]       = track.getAsignacionFc();
    obj["codigo_asignacion"]   = track.getCodigoAsignacion();
    obj["link_y"]              = static_cast<int>(track.getEstadoLinkY());
    obj["link_14"]             = static_cast<int>(track.getEstadoLink14());
    obj["codigo_privado"]      = track.getCodigoPrivado();
    return obj;
}

Track JsonSerializer::deserializeTrack(const QJsonObject& obj)
{
    auto parseType = [](const QString& s) {
        TrackData::Type t = TrackData::SPC;
        TrackData::tryParseType(s, t);
        return t;
    };

    auto parseIdentity = [](const QString& s) -> TrackData::Identity {
        const auto me = QMetaEnum::fromType<TrackData::Identity>();
        const int v   = me.keyToValue(qPrintable(s));
        return v >= 0 ? static_cast<TrackData::Identity>(v) : TrackData::Pending;
    };

    auto parseMode = [](const QString& s) -> TrackData::TrackMode {
        const auto me = QMetaEnum::fromType<TrackData::TrackMode>();
        const int v   = me.keyToValue(qPrintable(s));
        return v >= 0 ? static_cast<TrackData::TrackMode>(v) : TrackData::Auto;
    };

    Track track(
        0,  // id temporal — CommandContext asigna el id local en injectRemoteTrack
        parseType(obj["type"].toString()),
        parseIdentity(obj["identity"].toString()),
        parseMode(obj["mode"].toString()),
        static_cast<float>(obj["x"].toDouble()),
        static_cast<float>(obj["y"].toDouble()),
        0.0,  // speedKnots — se sobreescribe con setVelocidadDmPerHour
        obj["course_deg"].toInt(),
        parseType(obj["creation_environment"].toString())
    );

    track.setGuid(obj["guid"].toString().toStdString());
    track.setVelocidadDmPerHour(obj["speed_dm_h"].toDouble());
    track.setInformacionAmpliatoria(obj["info"].toString(QStringLiteral("-")));
    track.setAsignacionFc(obj["asignacion_fc"].toInt(0));
    track.setCodigoAsignacion(obj["codigo_asignacion"].toString(QStringLiteral("-")));
    track.setEstadoLinkY(static_cast<Track::LinkYStatus>(
        obj["link_y"].toInt(static_cast<int>(Track::LinkY_Invalid))));
    track.setEstadoLink14(static_cast<Track::Link14Status>(
        obj["link_14"].toInt(static_cast<int>(Track::Link14_Invalid))));
    track.setCodigoPrivado(obj["codigo_privado"].toString(QStringLiteral("-")));

    return track;
}
