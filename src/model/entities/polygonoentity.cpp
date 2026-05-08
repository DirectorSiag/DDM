#include "polygonoentity.h"
#include "model/utils/RadarMath.h"
#include "model/commandContext.h" // Necesario para la definición completa de CommandContext

PolygonoEntity::PolygonoEntity(int id, const std::vector<QPointF>& points, int type, const QString& color)
    : id(id), points(points), type(type), color(color) {
}

void PolygonoEntity::calculateAndStoreCursors(CommandContext& ctx) {
    if (points.size() < 2) return; // Necesitamos al menos 2 puntos para un segmento.

    cursorIds.clear();

    for (size_t i = 0; i < points.size(); ++i) {
        const QPointF& start = points[i];
        // Conectar con el siguiente, y el último con el primero para cerrar el polígono
        const QPointF& end = points[(i + 1) % points.size()]; 

        // Usamos RadarMath para los cálculos, igual que en AreaEntity
        qfloat16 angle = RadarMath::calculateAngle(start, end);
        qfloat16 length = RadarMath::calculateLength(start, end);

        CursorEntity cursor(
            QPair<qfloat16, qfloat16>(start.x(), start.y()), 
            angle, 
            length, 
            type, 
            ctx.nextCursorId++, 
            true
        );

        ctx.addCursorFront(cursor);
        cursorIds.push_back(cursor.getCursorId());
    }
}

int PolygonoEntity::getId() const {
    return id;
}

const std::vector<QPointF>& PolygonoEntity::getPoints() const {
    return points;
}

void PolygonoEntity::setPoints(const std::vector<QPointF>& newPoints) {
    points = newPoints;
}

int PolygonoEntity::getType() const {
    return type;
}

void PolygonoEntity::setType(int newType) {
    type = newType;
}

const QString& PolygonoEntity::getColor() const {
    return color;
}

void PolygonoEntity::setColor(const QString& newColor) {
    color = newColor;
}

const std::vector<int>& PolygonoEntity::getCursorIds() const {
    return cursorIds;
}

void PolygonoEntity::update(CommandContext& ctx) {
    // Si ya existen cursores, deberíamos eliminarlos del contexto antes de crear nuevos
    // para evitar duplicados "fantasmas".
    for (int cid : cursorIds) {
        ctx.eraseCursorById(cid);
    }
    
    // Recalcular y crear nuevos cursores
    calculateAndStoreCursors(ctx);
}
