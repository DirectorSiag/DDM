#include "circleEntity.h"
#include "model/utils/RadarMath.h"
#include "model/commandContext.h"
#include <QtMath>

CircleEntity::CircleEntity(int id, const QPointF& center, double radius, int type, const QString& color)
    : id(id), center(center), radius(radius), type(type), color(color) {
}

void CircleEntity::calculateAndStoreCursors(CommandContext& ctx) {
    if (radius <= 0) return;

    cursorIds.clear();

    const int segments = 36; // Aproximación con 36 segmentos (cada 10 grados)
    const double step = 2.0 * M_PI / segments;

    for (int i = 0; i < segments; ++i) {
        double angle1 = i * step;
        double angle2 = (i + 1) * step;

        // Calcular puntos del segmento en coordenadas cartesianas (ajustando para la pantalla si es necesario)
        // Asumiendo sistema estándar trigonométrico para la generación de puntos
        // (el ángulo absoluto del segmento será calculado por RadarMath después)
        
        QPointF p1(center.x() + radius * qCos(angle1), center.y() + radius * qSin(angle1));
        QPointF p2(center.x() + radius * qCos(angle2), center.y() + radius * qSin(angle2));

        qfloat16 angle = RadarMath::calculateAngle(p1, p2);
        qfloat16 length = RadarMath::calculateLength(p1, p2);

        CursorEntity cursor(
            QPair<qfloat16, qfloat16>(p1.x(), p1.y()), 
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

int CircleEntity::getId() const {
    return id;
}

const QPointF& CircleEntity::getCenter() const {
    return center;
}

double CircleEntity::getRadius() const {
    return radius;
}

int CircleEntity::getType() const {
    return type;
}

const QString& CircleEntity::getColor() const {
    return color;
}

const std::vector<int>& CircleEntity::getCursorIds() const {
    return cursorIds;
}

void CircleEntity::setCenter(const QPointF& center) {
    this->center = center;
}

void CircleEntity::setRadius(double radius) {
    this->radius = radius;
}

void CircleEntity::setType(int type) {
    this->type = type;
}

void CircleEntity::setColor(const QString& color) {
    this->color = color;
}
