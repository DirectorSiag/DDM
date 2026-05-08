#include "addareacommand.h"
#include "../services/geometryservice.h"
#include <QString>
#include <vector>

CommandResult AddAreaCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    // 1. Verificar que tengamos la cantidad correcta de argumentos
    // ax, ay, bx, by, cx, cy, dx, dy (8) + tipo (1) + color (1) = 10 argumentos
    if (inv.args.size() < 10) {
        return {false, "Error: Faltan argumentos. Uso: addArea(ax,ay,bx,by,cx,cy,dx,dy,tipo,color)"};
    }

    try {
        // Helper lambda to convert QString to double with validation
        auto validateAndConvert = [](const QString& value, const QString& fieldName) -> double {
            bool ok = false;
            double result = value.toDouble(&ok);
            if (!ok) {
                throw std::invalid_argument(("No se pudo convertir '" + fieldName + "' a un número: " + value).toStdString());
            }
            return result;
        };

        // 2. Extraer y convertir las coordenadas (los primeros 8 argumentos)
        double ax = validateAndConvert(inv.args[0], "ax");
        double ay = validateAndConvert(inv.args[1], "ay");
        double bx = validateAndConvert(inv.args[2], "bx");
        double by = validateAndConvert(inv.args[3], "by");
        double cx = validateAndConvert(inv.args[4], "cx");
        double cy = validateAndConvert(inv.args[5], "cy");
        double dx = validateAndConvert(inv.args[6], "dx");
        double dy = validateAndConvert(inv.args[7], "dy");

        // 3. Extraer tipo de área y color
        int areaType = 0;
        {
            bool ok = false;
            areaType = inv.args[8].toInt(&ok);
            if(!ok) throw std::invalid_argument("El tipo de área debe ser un entero.");
        }
        
        // El color ya es QString (según el error anterior inv.args es vector<QString>)
        QString areaColor = inv.args[9]; 

        GeometryService geometryService(&ctx);
        GeometryResult result = geometryService.createArea(
            {QPointF(ax, ay), QPointF(bx, by), QPointF(cx, cy), QPointF(dx, dy)},
            areaType,
            areaColor
        );

        if (!result.success) {
            return {false, QString("Error al crear area: %1").arg(result.message)};
        }

        return {true, QString("Area %1 creada exitosamente con los argumentos provistos.").arg(result.id)};

    } catch (const std::exception& e) {
        return {false, QString("Error al procesar argumentos: %1. Asegúrese de usar números para coordenadas y tipo.").arg(e.what())};
    }
}
