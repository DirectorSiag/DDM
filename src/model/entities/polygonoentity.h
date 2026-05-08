#ifndef POLYGONOENTITY_H
#define POLYGONOENTITY_H

#include <QString>
#include <QPointF>
#include <vector>
#include "model/entities/cursorEntity.h"
#include <QtMath>

// Forward declaration
struct CommandContext;

class PolygonoEntity {
public:
    PolygonoEntity(int id, const std::vector<QPointF>& points, int type, const QString& color);

    // Calcula y almacena los cursores (lineas) que forman el polígono
    void calculateAndStoreCursors(CommandContext& ctx);

    int getId() const;

    const std::vector<QPointF>& getPoints() const;
    void setPoints(const std::vector<QPointF>& points);

    int getType() const;
    void setType(int type);

    const QString& getColor() const;
    void setColor(const QString& color);

    const std::vector<int>& getCursorIds() const;
    
    // Método para recalcular cursores (alias de calculateAndStoreCursors para claridad)
    void update(CommandContext& ctx);

private:
    int id;
    std::vector<QPointF> points;
    int type;
    QString color;

    std::vector<int> cursorIds;
};

#endif // POLYGONOENTITY_H
