#include "geometryservice.h"

#include "commandContext.h"
#include "model/entities/areaEntity.h"
#include "model/entities/circleEntity.h"
#include "model/entities/polygonoentity.h"

#include <QJsonArray>

GeometryService::GeometryService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

GeometryResult GeometryService::createArea(const std::vector<QPointF>& points, int areaType, const QString& areaColor)
{
    if (points.size() != 4) {
        return {false, "INVALID_POINTS", "Un area requiere exactamente 4 puntos", -1};
    }

    AreaEntity area(m_context->commandCounter++, points, areaType, areaColor);
    area.calculateAndStoreCursors(*m_context);
    m_context->addArea(area);
    return {true, QString(), QString(), area.getId()};
}

GeometryResult GeometryService::deleteArea(int areaId)
{
    if (areaId < 0) {
        return {false, "INVALID_ID", "El id del area debe ser no negativo", -1};
    }
    if (!m_context->deleteArea(areaId)) {
        return {false, "NOT_FOUND", QString("No se encontro un area con ID %1").arg(areaId), areaId};
    }
    return {true, QString(), QString(), areaId};
}

GeometryResult GeometryService::createCircle(const QPointF& center, double radius, int type, const QString& color)
{
    if (radius <= 0.0) {
        return {false, "INVALID_RADIUS", "El radio debe ser mayor a 0", -1};
    }

    CircleEntity circle(m_context->commandCounter++, center, radius, type, color);
    circle.calculateAndStoreCursors(*m_context);
    m_context->addCircle(circle);
    return {true, QString(), QString(), circle.getId()};
}

GeometryResult GeometryService::deleteCircle(int circleId)
{
    if (circleId < 0) {
        return {false, "INVALID_ID", "El id del circulo debe ser no negativo", -1};
    }
    if (!m_context->deleteCircle(circleId)) {
        return {false, "NOT_FOUND", QString("No se encontro un circulo con ID %1").arg(circleId), circleId};
    }
    return {true, QString(), QString(), circleId};
}

GeometryResult GeometryService::createPolygon(const std::vector<QPointF>& points, int polyType, const QString& polyColor)
{
    if (points.size() < 3) {
        return {false, "INVALID_POINTS", "Un poligono requiere al menos 3 puntos", -1};
    }

    PolygonoEntity polygon(m_context->commandCounter++, points, polyType, polyColor);
    polygon.calculateAndStoreCursors(*m_context);
    m_context->addPolygon(polygon);
    return {true, QString(), QString(), polygon.getId()};
}

GeometryResult GeometryService::deletePolygon(int polygonId)
{
    if (polygonId < 0) {
        return {false, "INVALID_ID", "El id del poligono debe ser no negativo", -1};
    }
    if (!m_context->deletePolygon(polygonId)) {
        return {false, "NOT_FOUND", QString("No se encontro un poligono con ID %1").arg(polygonId), polygonId};
    }
    return {true, QString(), QString(), polygonId};
}

QJsonObject GeometryService::listShapes() const
{
    QJsonObject out;

    QJsonArray areasArray;
    for (const AreaEntity& area : m_context->getAreas()) {
        QJsonObject obj;
        obj["id"] = area.getId();
        obj["type"] = area.getType();
        obj["color"] = area.getColor();
        QJsonArray cursorIds;
        cursorIds.append(area.getCursorIdAB());
        cursorIds.append(area.getCursorIdBC());
        cursorIds.append(area.getCursorIdCD());
        cursorIds.append(area.getCursorIdDA());
        obj["cursor_ids"] = cursorIds;
        areasArray.append(obj);
    }
    out["areas"] = areasArray;

    QJsonArray circlesArray;
    for (const CircleEntity& circle : m_context->getCircles()) {
        QJsonObject obj;
        obj["id"] = circle.getId();
        obj["type"] = circle.getType();
        obj["color"] = circle.getColor();
        QJsonArray cursorIds;
        for (int cid : circle.getCursorIds()) {
            cursorIds.append(cid);
        }
        obj["cursor_ids"] = cursorIds;
        circlesArray.append(obj);
    }
    out["circles"] = circlesArray;

    QJsonArray polygonsArray;
    for (const PolygonoEntity& polygon : m_context->getPolygons()) {
        QJsonObject obj;
        obj["id"] = polygon.getId();
        obj["type"] = polygon.getType();
        obj["color"] = polygon.getColor();
        QJsonArray cursorIds;
        for (int cid : polygon.getCursorIds()) {
            cursorIds.append(cid);
        }
        obj["cursor_ids"] = cursorIds;
        polygonsArray.append(obj);
    }
    out["polygons"] = polygonsArray;

    return out;
}
