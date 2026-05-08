#include "areaEntity.h"
#include "model/utils/RadarMath.h"
#include "model/commandContext.h"

AreaEntity::AreaEntity(int id, const std::vector<QPointF>& points, int type, const QString& color)
    : id(id), type(type), color(color) {
    if (points.size() >= 4) {
        pointA = points[0];
        pointB = points[1];
        pointC = points[2];
        pointD = points[3];
    }
}



//getters
int AreaEntity::getId() const {
    return id;
}

const QPointF& AreaEntity::getPointA() const {
    return pointA;
}

const QPointF& AreaEntity::getPointB() const {
    return pointB;
}

const QPointF& AreaEntity::getPointC() const {
    return pointC;
}

const QPointF& AreaEntity::getPointD() const {
    return pointD;
}

const QString& AreaEntity::getColor() const {
    return color;
}

int AreaEntity::getType() const {
    return type;
}

int AreaEntity::getCursorIdAB() const {
    return cursorIdAB;
}

int AreaEntity::getCursorIdBC() const {
    return cursorIdBC;
}

int AreaEntity::getCursorIdCD() const {
    return cursorIdCD;
}

int AreaEntity::getCursorIdDA() const {
    return cursorIdDA;
}

//setters
void AreaEntity::setPointA(const QPointF& point) {
    pointA = point;

    // // Recalculate cursor DA
    // CursorEntity cursorDA(QPair<qfloat16, qfloat16>(pointD.x(), pointD.y()), calculateAngle(pointD, pointA), calculateLength(pointD, pointA), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorDA);
    // cursorIdDA = cursorDA.getCursorId();

    // // Recalculate cursor AB
    // CursorEntity cursorAB(QPair<qfloat16, qfloat16>(pointA.x(), pointA.y()), calculateAngle(pointA, pointB), calculateLength(pointA, pointB), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorAB);
    // cursorIdAB = cursorAB.getCursorId();
}

void AreaEntity::setPointB(const QPointF& point) {
    pointB = point;

    // // Recalculate cursor AB
    // CursorEntity cursorAB(QPair<qfloat16, qfloat16>(pointA.x(), pointA.y()), calculateAngle(pointA, pointB), calculateLength(pointA, pointB), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorAB);
    // cursorIdAB = cursorAB.getCursorId();

    // // Recalculate cursor BC
    // CursorEntity cursorBC(QPair<qfloat16, qfloat16>(pointB.x(), pointB.y()), calculateAngle(pointB, pointC), calculateLength(pointB, pointC), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorBC);
    // cursorIdBC = cursorBC.getCursorId();
}

void AreaEntity::setPointC(const QPointF& point) {
    pointC = point;

    // // Recalculate cursor BC
    // CursorEntity cursorBC(QPair<qfloat16, qfloat16>(pointB.x(), pointB.y()), calculateAngle(pointB, pointC), calculateLength(pointB, pointC), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorBC);
    // cursorIdBC = cursorBC.getCursorId();

    // // Recalculate cursor CD
    // CursorEntity cursorCD(QPair<qfloat16, qfloat16>(pointC.x(), pointC.y()), calculateAngle(pointC, pointD), calculateLength(pointC, pointD), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorCD);
    // cursorIdCD = cursorCD.getCursorId();
}

void AreaEntity::setPointD(const QPointF& point) {
    pointD = point;

    // // Recalculate cursor CD
    // CursorEntity cursorCD(QPair<qfloat16, qfloat16>(pointC.x(), pointC.y()), calculateAngle(pointC, pointD), calculateLength(pointC, pointD), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorCD);
    // cursorIdCD = cursorCD.getCursorId();

    // // Recalculate cursor DA
    // CursorEntity cursorDA(QPair<qfloat16, qfloat16>(pointD.x(), pointD.y()), calculateAngle(pointD, pointA), calculateLength(pointD, pointA), 1, ctx.nextCursorId++, true);
    // ctx.addCursorFront(cursorDA);
    // cursorIdDA = cursorDA.getCursorId();
}

void AreaEntity::setType(int newType) {
    type = newType;
}

void AreaEntity::setColor(const QString& newColor) {
    color = newColor;
}

void AreaEntity::setCursorIdAB(int cursorId) {
    cursorIdAB = cursorId;
}

void AreaEntity::setCursorIdBC(int cursorId) {
    cursorIdBC = cursorId;
}

void AreaEntity::setCursorIdCD(int cursorId) {
    cursorIdCD = cursorId;
}

void AreaEntity::setCursorIdDA(int cursorId) {
    cursorIdDA = cursorId;
}

void AreaEntity::calculateAndStoreCursors(CommandContext& ctx) {
    using RadarMath = RadarMath; // Or just use RadarMath::
    // Calculate and store cursor IDs for each edge
    CursorEntity cursorAB(QPair<qfloat16, qfloat16>(pointA.x(), pointA.y()), RadarMath::calculateAngle(pointA, pointB), RadarMath::calculateLength(pointA, pointB), type, ctx.nextCursorId++, true);
    ctx.addCursorFront(cursorAB);
    cursorIdAB = cursorAB.getCursorId();

    CursorEntity cursorBC(QPair<qfloat16, qfloat16>(pointB.x(), pointB.y()), RadarMath::calculateAngle(pointB, pointC), RadarMath::calculateLength(pointB, pointC), type, ctx.nextCursorId++, true);
    ctx.addCursorFront(cursorBC);
    cursorIdBC = cursorBC.getCursorId();

    CursorEntity cursorCD(QPair<qfloat16, qfloat16>(pointC.x(), pointC.y()), RadarMath::calculateAngle(pointC, pointD), RadarMath::calculateLength(pointC, pointD), type, ctx.nextCursorId++, true);
    ctx.addCursorFront(cursorCD);
    cursorIdCD = cursorCD.getCursorId();

    CursorEntity cursorDA(QPair<qfloat16, qfloat16>(pointD.x(), pointD.y()), RadarMath::calculateAngle(pointD, pointA), RadarMath::calculateLength(pointD, pointA), type, ctx.nextCursorId++, true);
    ctx.addCursorFront(cursorDA);
    cursorIdDA = cursorDA.getCursorId();
}

// qfloat16 AreaEntity::calculateAngle(const QPointF& start, const QPointF& end) const {
//     // Invertimos Y porque en coordenadas de pantalla Y crece hacia abajo,
//     // mientras que atan2 asume coordenadas cartesianas donde Y crece hacia arriba.
//     return qAtan2(-(end.y() - start.y()), end.x() - start.x()) * (180.0 / M_PI);
// }

// qfloat16 AreaEntity::calculateLength(const QPointF& start, const QPointF& end) const {
//     return qSqrt(qPow(end.x() - start.x(), 2) + qPow(end.y() - start.y(), 2));
// }




