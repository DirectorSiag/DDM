#ifndef AREAENTITY_H
#define AREAENTITY_H

#include <QString>
#include <QPointF>
#include <vector>
#include "model/entities/cursorEntity.h"
#include <QtMath>

// Cambiar class por struct para que coincida con la definición en commandContext.h
struct CommandContext;

class AreaEntity {
public:
    AreaEntity(int id, const std::vector<QPointF>& points, int type, const QString& color);
    
    void calculateAndStoreCursors(CommandContext& ctx);

    int getId() const;
    const std::vector<QPointF>& getPoints() const;
    
    void setPoints(const std::vector<QPointF>& points);

    const QPointF& getPointA() const;
    void setPointA(const QPointF& point);

    const QPointF& getPointB() const;
    void setPointB(const QPointF& point);

    const QPointF& getPointC() const;
    void setPointC(const QPointF& point);

    const QPointF& getPointD() const;
    void setPointD(const QPointF& point);

    int getType() const;
    void setType(int type);

    const QString& getColor() const;
    void setColor(const QString& color);

    int getCursorIdAB() const;
    void setCursorIdAB(int cursorId);

    int getCursorIdBC() const;
    void setCursorIdBC(int cursorId);

    int getCursorIdCD() const;
    void setCursorIdCD(int cursorId);

    int getCursorIdDA() const;
    void setCursorIdDA(int cursorId);

private:
    int id;
    QPointF pointA;
    QPointF pointB;
    QPointF pointC;
    QPointF pointD;

    int type;
    QString color;

    int cursorIdAB;
    int cursorIdBC;
    int cursorIdCD;
    int cursorIdDA;

    // qfloat16 calculateAngle(const QPointF& start, const QPointF& end) const;
    // qfloat16 calculateLength(const QPointF& start, const QPointF& end) const;
};

#endif // AREAENTITY_H
