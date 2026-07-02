#include "deleteSectorCommand.h"
#include "../services/geometryservice.h"
#include <QString>

CommandResult DeleteSectorCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    if (inv.args.size() != 1) {
        return {false, "Error: Se requiere exactamente un argumento (el ID del sector). Uso: deleteSector(id)"};
    }

    bool ok = false;
    int sectorId = inv.args[0].toInt(&ok);
    if (!ok) {
        return {false, "Error: El ID del sector debe ser un número entero."};
    }

    GeometryService geometryService(&ctx);
    GeometryResult result = geometryService.deleteSector(sectorId);
    if (result.success) {
        return {true, QString("Sector %1 eliminado exitosamente.").arg(sectorId)};
    } else {
        return {false, QString("Error: No se encontró un sector con ID %1.").arg(sectorId)};
    }
}
