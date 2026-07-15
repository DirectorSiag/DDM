#include "replicationEngine/stubReplicationEngine.h"

#include <QDebug>

void StubReplicationEngine::start()
{
    qInfo() << "[StubRE] start()";
}

void StubReplicationEngine::stop()
{
    qInfo() << "[StubRE] stop()";
}

void StubReplicationEngine::onLocalObjectUpserted(const replication_engine::ReplicatedObject& obj)
{
    qInfo() << "[StubRE] onLocalObjectUpserted guid:" << QString::fromStdString(obj.guid)
            << "type:" << obj.object_type
            << "console:" << obj.source_console_id
            << "last_updated:" << obj.last_updated
            << "payload (" << obj.json_payload.size() << "bytes):"
            << QString::fromStdString(obj.json_payload);
}

void StubReplicationEngine::onLocalObjectDeleted(const std::string& guid)
{
    qInfo() << "[StubRE] onLocalObjectDeleted guid:" << QString::fromStdString(guid);
}

void StubReplicationEngine::reconnectTransport()
{
    qInfo() << "[StubRE] reconnectTransport()";
}

void StubReplicationEngine::reinitializeStorage()
{
    qInfo() << "[StubRE] reinitializeStorage()";
}
