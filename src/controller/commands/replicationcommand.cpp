#include "replicationcommand.h"

#include <QDateTime>

#include "entities/track.h"
#include "replication/replicationbridge.h"
#include "replication/tracknetserializer.h"
#include "stub/StubReplicationEngine.h"

namespace {
// Consola remota ficticia desde la que "llegan" los eventos simulados.
constexpr int kFakeRemoteConsoleId = 99;
}

CommandResult ReplicationCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    Q_UNUSED(ctx);

    if (!m_stub) return {false, "StubReplicationEngine no disponible"};
    if (inv.args.isEmpty()) return {false, usage()};

    const QString sub = inv.args.first();
    const QStringList args = inv.args.mid(1);

    if (sub == QLatin1String("inject")) {
        if (args.size() < 3) return {false, "uso: rep inject <guid> <x> <y> [course] [speed_dmh] [identity]"};

        Track track;
        track.setX(args[1].toFloat());
        track.setY(args[2].toFloat());
        if (args.size() > 3) track.setCursoInt(args[3].toInt());
        if (args.size() > 4) track.setVelocidadDmPerHour(args[4].toDouble());
        if (args.size() > 5) track.setIdentity(static_cast<Track::Identity>(args[5].toInt()));
        track.setInformacionAmpliatoria(QStringLiteral("remoto-simulado"));

        StubReplicationEngine::ReplicatedObject obj;
        obj.guid = args[0].toStdString();
        obj.object_type = ReplicationBridge::kObjectTypeTrack;
        obj.source_console_id = kFakeRemoteConsoleId;
        obj.last_updated = QDateTime::currentMSecsSinceEpoch();
        obj.json_payload = TrackNetSerializer::toNetworkPayload(track).toStdString();

        m_stub->simulateRemoteUpsert(obj);
        return {true, QString("inyeccion remota simulada — guid: %1").arg(args[0])};
    }

    if (sub == QLatin1String("remove")) {
        if (args.size() != 1) return {false, "uso: rep remove <guid>"};
        m_stub->simulateRemoteDelete(args[0].toStdString());
        return {true, QString("borrado remoto simulado — guid: %1").arg(args[0])};
    }

    if (sub == QLatin1String("status")) {
        if (args.size() != 1) return {false, "uso: rep status <0..4>"};
        bool ok = false;
        const int status = args[0].toInt(&ok);
        if (!ok || status < 0 || status > 4) return {false, "status debe ser 0..4"};
        m_stub->simulateNetworkStatus(status);
        return {true, QString("estado de red simulado: %1").arg(status)};
    }

    if (sub == QLatin1String("snapshot")) {
        const int batches = args.isEmpty() ? 1 : args[0].toInt();
        m_stub->simulateSnapshot(batches);
        return {true, "snapshot simulado"};
    }

    if (sub == QLatin1String("db")) {
        m_stub->dumpDb();
        return {true, "BD simulada volcada por stdout"};
    }

    return {false, usage()};
}
