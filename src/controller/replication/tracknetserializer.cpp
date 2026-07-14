#include "tracknetserializer.h"

#include <QJsonDocument>

#include "entities/track.h"

namespace TrackNetSerializer {

QByteArray toNetworkPayload(const Track& track)
{
    QJsonObject obj;
    obj["v"] = kSchemaVersion;
    obj["type"] = static_cast<int>(track.getType());
    obj["env"] = static_cast<int>(track.getCreationEnvironment());
    obj["identity"] = static_cast<int>(track.getIdentity());
    obj["mode"] = static_cast<int>(track.getTrackMode());
    obj["x_dm"] = static_cast<double>(track.getX());
    obj["y_dm"] = static_cast<double>(track.getY());
    obj["speed_dmh"] = track.getVelocidadDmPerHour();
    obj["course_deg"] = track.getCourseDeg();
    obj["info"] = track.getInformacionAmpliatoria();
    obj["fc"] = track.getAsignacionFc();
    obj["asgc"] = track.getCodigoAsignacion();
    obj["link_y"] = static_cast<int>(track.getEstadoLinkY());
    obj["link_14"] = static_cast<int>(track.getEstadoLink14());
    obj["priv"] = track.getCodigoPrivado();

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

bool parsePayload(const std::string& jsonPayload, QJsonObject& out)
{
    const QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray(jsonPayload.data(), static_cast<int>(jsonPayload.size())));
    if (!doc.isObject()) {
        return false;
    }
    out = doc.object();
    return true;
}

void applyPayload(const QJsonObject& payload, Track& track)
{
    if (payload.contains("type"))
        track.setType(static_cast<Track::Type>(payload["type"].toInt()));
    if (payload.contains("env"))
        track.setCreationEnvironment(static_cast<Track::Environment>(payload["env"].toInt()));
    if (payload.contains("identity"))
        track.setIdentity(static_cast<Track::Identity>(payload["identity"].toInt()));
    if (payload.contains("mode"))
        track.setTrackMode(static_cast<Track::TrackMode>(payload["mode"].toInt()));
    if (payload.contains("x_dm"))
        track.setX(static_cast<float>(payload["x_dm"].toDouble()));
    if (payload.contains("y_dm"))
        track.setY(static_cast<float>(payload["y_dm"].toDouble()));
    if (payload.contains("speed_dmh"))
        track.setVelocidadDmPerHour(payload["speed_dmh"].toDouble());
    if (payload.contains("course_deg"))
        track.setCursoInt(static_cast<int>(payload["course_deg"].toDouble()));
    if (payload.contains("info"))
        track.setInformacionAmpliatoria(payload["info"].toString());
    if (payload.contains("fc"))
        track.setAsignacionFc(payload["fc"].toInt());
    if (payload.contains("asgc"))
        track.setCodigoAsignacion(payload["asgc"].toString());
    if (payload.contains("link_y"))
        track.setEstadoLinkY(static_cast<Track::LinkYStatus>(payload["link_y"].toInt()));
    if (payload.contains("link_14"))
        track.setEstadoLink14(static_cast<Track::Link14Status>(payload["link_14"].toInt()));
    if (payload.contains("priv"))
        track.setCodigoPrivado(payload["priv"].toString());
}

} // namespace TrackNetSerializer
