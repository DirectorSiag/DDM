/*
    Comando `rep`: herramienta de prueba del flujo de replicación con el
    StubReplicationEngine. Simula eventos remotos (inyección, borrado,
    estado de red, snapshot) y permite inspeccionar la BD simulada.

    ⚠️ Se elimina junto con el stub cuando SiOp entregue el .so real.
*/
#pragma once
#include "iCommand.h"

class StubReplicationEngine;

class ReplicationCommand : public ICommand {
public:
    explicit ReplicationCommand(StubReplicationEngine* stub) : m_stub(stub) {}

    QString getName() const override { return "rep"; }
    QString getDescription() const override {
        return "Simula eventos de replicacion (stub de ReplicationEngine)";
    }
    QString usage() const override {
        return "rep inject <guid> <x> <y> [course] [speed_dmh] [identity]\n"
               "rep remove <guid>\n"
               "rep status <0..4>   (0=DISC 1=CONN 2=SYNC 3=TRANSPORT_FAILED 4=STORAGE_FAILED)\n"
               "rep snapshot [batches]\n"
               "rep db";
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;

private:
    StubReplicationEngine* m_stub;
};
