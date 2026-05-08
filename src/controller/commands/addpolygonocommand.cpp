#include "addpolygonocommand.h"
#include "../services/geometryservice.h"
#include <iostream>
#include <vector>
#include <QString>
#include <stdexcept>

CommandResult AddPolygonoCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    // 1. Verificar argumentos mínimos
    // Necesitamos al menos 3 puntos -> 6 coordenadas
    // + tipo + color = 8 argumentos mínimos
    // Además, el total de argumentos debe ser par (2 * N + 2) -> Siempre par.
    
    if (inv.args.size() < 8) {
        return {false, "Error: Argumentos insuficientes. Se requieren al menos 3 puntos (6 coord), tipo y color.\nUso: " + usage()};
    }

    if (inv.args.size() % 2 != 0) {
        return {false, "Error: Cantidad incorrecta de argumentos. Las coordenadas deben venir en pares (x, y), mas 2 finales (tipo, color)."};
    }

    try {
        // Helper lambda para validar double
        auto validateAndConvert = [](const QString& value, int index) -> double {
            bool ok = false;
            double result = value.toDouble(&ok);
            if (!ok) {
                // Generar un mensaje de error claro
                throw std::invalid_argument(("No se pudo convertir el argumento " + QString::number(index+1) + " ('" + value + "') a un número.").toStdString());
            }
            return result;
        };

        std::vector<QPointF> points;
        
        // Los últimos 2 argumentos son tipo y color
        // Iteramos hasta size - 2
        int numArgs = inv.args.size();
        int coordsLimit = numArgs - 2;

        for (int i = 0; i < coordsLimit; i += 2) {
            double x = validateAndConvert(inv.args[i], i);
            double y = validateAndConvert(inv.args[i+1], i+1);
            points.emplace_back(x, y);
        }

        // Extraer tipo y color (últimos 2)
        int typeIdx = coordsLimit;      // Penúltimo
        int colorIdx = coordsLimit + 1; // Último

        int polyType = 0;
        {
            bool ok = false;
            polyType = inv.args[typeIdx].toInt(&ok);
            if(!ok) throw std::invalid_argument("El tipo de polígono (penúltimo argumento) debe ser un entero.");
        }

        QString polyColor = inv.args[colorIdx];

        GeometryService geometryService(&ctx);
        GeometryResult result = geometryService.createPolygon(points, polyType, polyColor);
        if (!result.success) {
            return {false, QString("Error al crear poligono: %1").arg(result.message)};
        }

        return {true, QString("Poligono %1 creado exitosamente con %2 puntos.").arg(result.id).arg(points.size())};

    } catch (const std::exception& e) {
        return {false, QString("Error al procesar argumentos: %1").arg(e.what())};
    }
}
