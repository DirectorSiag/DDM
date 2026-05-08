#include "geometrycommandhandler.h"

#include "../json/jsonresponsebuilder.h"
#include "../services/geometryservice.h"
#include "../services/obmservice.h"
#include "commandContext.h"
#include "network/iTransport.h"

#include <QJsonArray>

GeometryCommandHandler::GeometryCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService)
    : m_context(context), m_transport(transport), m_obmService(obmService), m_geometryService(std::make_unique<GeometryService>(context))
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_transport);
    Q_ASSERT(m_obmService);
}

QByteArray GeometryCommandHandler::createArea(const QJsonObject& args)
{
    if (!args.contains("points") || !args.value("points").isArray()) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_area", "points", "", "array requerido");
    }

    QJsonArray pointsArray = args.value("points").toArray();
    if (pointsArray.size() != 4) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_area", "points", QString::number(pointsArray.size()), "exactamente 4 puntos");
    }

    std::vector<QPointF> points;
    for (const QJsonValue& value : pointsArray) {
        if (!value.isObject()) {
            return JsonResponseBuilder::buildValidationErrorResponse("create_area", "points", "", "objeto {x,y} requerido");
        }
        QJsonObject p = value.toObject();
        points.emplace_back(p.value("x").toDouble(), p.value("y").toDouble());
    }

    const int type = args.value("type").toInt(0);
    const QString color = args.value("color").toString("ROJO");

    GeometryResult result = m_geometryService->createArea(points, type, color);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("create_area", result.errorCode, result.message);
    }

    return JsonResponseBuilder::buildSuccessResponse("create_area", QJsonObject{{"created_id", result.id}});
}

QByteArray GeometryCommandHandler::deleteArea(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_area", "id", "", "required");
    }

    const int id = args.value("id").toInt(-1);
    GeometryResult result = m_geometryService->deleteArea(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_area", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("delete_area", QJsonObject{{"deleted_id", id}});
}

QByteArray GeometryCommandHandler::createCircle(const QJsonObject& args)
{
    if (!args.contains("radius")) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_circle", "radius", "", "required");
    }

    const auto obmPosition = m_obmService->getCurrentPosition();
    const double x = obmPosition.first;
    const double y = obmPosition.second;
    const double radius = args.value("radius").toDouble(-1.0);
    const int type = args.value("type").toInt(0);
    const QString color = args.value("color").toString("ROJO");

    GeometryResult result = m_geometryService->createCircle(QPointF(x, y), radius, type, color);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("create_circle", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("create_circle", QJsonObject{{"created_id", result.id}});
}

QByteArray GeometryCommandHandler::deleteCircle(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_circle", "id", "", "required");
    }

    const int id = args.value("id").toInt(-1);
    GeometryResult result = m_geometryService->deleteCircle(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_circle", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("delete_circle", QJsonObject{{"deleted_id", id}});
}

QByteArray GeometryCommandHandler::createPolygon(const QJsonObject& args)
{
    if (!args.contains("points") || !args.value("points").isArray()) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_polygon", "points", "", "array requerido");
    }

    QJsonArray pointsArray = args.value("points").toArray();
    std::vector<QPointF> points;
    for (const QJsonValue& value : pointsArray) {
        if (!value.isObject()) {
            return JsonResponseBuilder::buildValidationErrorResponse("create_polygon", "points", "", "objeto {x,y} requerido");
        }
        QJsonObject p = value.toObject();
        points.emplace_back(p.value("x").toDouble(), p.value("y").toDouble());
    }

    const int type = args.value("type").toInt(0);
    const QString color = args.value("color").toString("ROJO");

    GeometryResult result = m_geometryService->createPolygon(points, type, color);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("create_polygon", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("create_polygon", QJsonObject{{"created_id", result.id}});
}

QByteArray GeometryCommandHandler::deletePolygon(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_polygon", "id", "", "required");
    }

    const int id = args.value("id").toInt(-1);
    GeometryResult result = m_geometryService->deletePolygon(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_polygon", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("delete_polygon", QJsonObject{{"deleted_id", id}});
}

QByteArray GeometryCommandHandler::listShapes(const QJsonObject& /*args*/)
{
    return JsonResponseBuilder::buildSuccessResponse("list_shapes", m_geometryService->listShapes());
}
