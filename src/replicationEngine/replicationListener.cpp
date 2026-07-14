#include "replicationEngine/replicationListener.h"

#include "entities/track.h"
#include "enums.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

// Inverso del campo "link" de serializeTracks(): "R"/"C"/"T"/"S" → enum,
// "--" u otro valor → ok=false.
Track::LinkYStatus linkYFromCode(const QString& code, bool& ok) {
    ok = true;
    const QString c = code.trimmed().toUpper();
    if (c == "R") return Track::LinkY_ReceivedOnly;
    if (c == "C") return Track::LinkY_Correlated;
    if (c == "T") return Track::LinkY_Tx;
    if (c == "S") return Track::LinkY_TxS;
    ok = false;
    return Track::LinkY_Invalid;
}

// Desempaqueta el json_payload del Envelope validando que sea JSON bien formado.
bool parsePayload(const std::string& payload, QJsonObject& out) {
    QJsonParseError err;
    const QJsonDocument doc =
        QJsonDocument::fromJson(QByteArray::fromStdString(payload), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    out = doc.object();
    return true;
}

// Convierte el payload (formato serializeTracks) en un TrackCreateRequest.
// Mapeo inverso: lon → x, lat → y; azimut/distancia/ppp_* se ignoran (derivados).
TrackCreateRequest trackRequestFromJson(const QJsonObject& json) {
    TrackCreateRequest request;

    TrackData::Type type = TrackData::SPC;
    if (TrackData::tryParseType(json.value("type").toString(), type)) {
        request.type = type;
    }

    TrackData::Identity identity = TrackData::Pending;
    if (TrackData::tryParseIdentity(json.value("identity").toString(), identity)) {
        request.identity = identity;
    }

    request.x = json.value("lon").toDouble();
    request.y = json.value("lat").toDouble();

    if (json.contains("velocidad")) request.speedDmPerHour = json.value("velocidad").toDouble();
    if (json.contains("rumbo"))     request.courseDeg = json.value("rumbo").toInt();
    if (json.contains("info"))      request.info = json.value("info").toString();

    bool linkOk = false;
    const Track::LinkYStatus linkY = linkYFromCode(json.value("link").toString(), linkOk);
    if (linkOk) request.linkY = linkY;

    return request;
}

} // namespace

ReplicationListener::ReplicationListener(QObject* parent)
    : QObject(parent)
{
    // Habilita TrackCreateRequest como argumento de señal en conexiones encoladas.
    qRegisterMetaType<TrackCreateRequest>("TrackCreateRequest");
}

// ---------------------------------------------------------------------------
// Callbacks de iReplicationListener — invocados desde el Worker Thread de RE.
// Emitir señales es thread-safe: la conexión encolada entrega el slot en el
// hilo Qt del receptor (ICD §9.4).
// ---------------------------------------------------------------------------

void ReplicationListener::onInjectObject(const ReplicatedObject& obj)
{
    const QString guid = QString::fromStdString(obj.guid);

    QJsonObject payload;
    if (!parsePayload(obj.json_payload, payload)) {
        qWarning() << "ReplicationListener: json_payload inválido, guid:" << guid;
        return;
    }

    switch (obj.object_type) {
    case ReplicationData::Track:
        emit trackReceived(guid, trackRequestFromJson(payload));
        break;
    // Futuros object_type: case ReplicationData::Area → emit areaReceived(...),
    // case ReplicationData::Cursor → emit cursorReceived(...), etc.
    default:
        qDebug() << "ReplicationListener: object_type no soportado:" << obj.object_type
                 << "guid:" << guid;
        break;
    }
}

void ReplicationListener::onRemoveObject(const std::string& guid)
{
    emit trackRemoved(QString::fromStdString(guid));
}

void ReplicationListener::onClearAllObjects()
{
    emit clearAllReceived();
}

// Pendientes de diseño (plan.md §5.8) — el feedback al operador se definirá
// cuando se conecte la UI.

void ReplicationListener::onSnapshotProgress(int current, int total)
{
    Q_UNUSED(current)
    Q_UNUSED(total)
}

void ReplicationListener::onSnapshotCompleted()
{
}

void ReplicationListener::onNetworkStatusChanged(int status)
{
    Q_UNUSED(status)
}
