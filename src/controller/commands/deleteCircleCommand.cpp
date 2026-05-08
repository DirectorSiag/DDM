#include "deleteCircleCommand.h"
#include "../services/geometryservice.h"
#include <QString>

CommandResult DeleteCircleCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    if (inv.args.size() != 1) {
        return {false, "Error: Se requiere exactamente un argumento (el ID del círculo). Uso: deleteCircle(id)"};
    }

    bool ok = false;
    int circleId = inv.args[0].toInt(&ok);
    if (!ok) {
        return {false, "Error: El ID del círculo debe ser un número entero."};
    }

    GeometryService geometryService(&ctx);
    GeometryResult result = geometryService.deleteCircle(circleId);
    if (result.success) {
        return {true, QString("Círculo %1 eliminado exitosamente.").arg(circleId)};
    } else {
        return {false, QString("Error: No se encontró un círculo con ID %1.").arg(circleId)};
    }
}
