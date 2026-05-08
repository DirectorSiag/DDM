#ifndef CIRCLEENTITY_H
#define CIRCLEENTITY_H

#include <QString>
#include <QPointF>
#include <QtMath>
#include "model/entities/cursorEntity.h"

// Forward declaration
struct CommandContext;

class CircleEntity {
public:
    CircleEntity(int id, const QPointF& center, double radius, int type, const QString& color);

    // Calcula y almacena los cursores (segmentos) que aproximan el círculo
    void calculateAndStoreCursors(CommandContext& ctx);

    int getId() const;
    const QPointF& getCenter() const;
    double getRadius() const;
    int getType() const;
    const QString& getColor() const;

    const std::vector<int>& getCursorIds() const;

    void setCenter(const QPointF& center);
    void setRadius(double radius);
    void setType(int type);
    void setColor(const QString& color);

private:
    int id;
    QPointF center;
    double radius;
    int type;
    QString color;

    // Almacena IDs de los cursores generados (opcional, si quisiéramos borrarlos después)
    std::vector<int> cursorIds;
};

#endif // CIRCLEENTITY_H
