#include "geometrycommandhandler.h"

#include "../json/jsonresponsebuilder.h"
#include "../services/geometryservice.h"
#include "../services/obmservice.h"
#include "commandContext.h"
#include "network/iTransport.h"

#include <QJsonArray>

namespace {

// Mismo mapeo que AddSectorCommand.cpp (parseSectorColor), duplicado aquí porque
// esa función es static dentro de su propio .cpp y no es reutilizable directamente.
bool parseSectorColorJson(const QString& s, SectorColor& out)
{
    if (s.compare("RGB1", Qt::CaseInsensitive) == 0) { out = SectorColor::RGB1; return true; }
    if (s.compare("RGB2", Qt::CaseInsensitive) == 0) { out = SectorColor::RGB2; return true; }
    if (s.compare("RGB3", Qt::CaseInsensitive) == 0) { out = SectorColor::RGB3; return true; }
    if (s.compare("CMYK1", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK1; return true; }
    if (s.compare("CMYK2", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK2; return true; }
    if (s.compare("CMYK3", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK3; return true; }
    return false;
}

}

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

QByteArray GeometryCommandHandler::createSector(const QJsonObject& args)
{
    if (!args.contains("az_izq") || !args.contains("az_der") ||
        !args.contains("rad_int") || !args.contains("rad_ext")) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_sector", "az_izq/az_der/rad_int/rad_ext", "", "requeridos");
    }

    SectorColor color;
    if (!parseSectorColorJson(args.value("color").toString(), color)) {
        return JsonResponseBuilder::buildValidationErrorResponse("create_sector", "color", args.value("color").toString(), "RGB1|RGB2|RGB3|CMYK1|CMYK2|CMYK3");
    }

    const QJsonObject origenObj = args.value("origen").toObject();

    SectorCreateRequest req;
    req.az_izq   = args.value("az_izq").toDouble();
    req.az_der   = args.value("az_der").toDouble();
    req.rad_int  = args.value("rad_int").toDouble();
    req.rad_ext  = args.value("rad_ext").toDouble();
    req.color    = color;
    req.origen   = QPointF(origenObj.value("x").toDouble(), origenObj.value("y").toDouble());
    req.id_track = args.value("id_track").toInt(0);

    GeometryResult result = m_geometryService->createSector(req);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("create_sector", result.errorCode, result.message);
    }

    return JsonResponseBuilder::buildSuccessResponse("create_sector", QJsonObject{{"created_id", result.id}});
}

QByteArray GeometryCommandHandler::deleteSector(const QJsonObject& args)
{
    if (!args.contains("id")) {
        return JsonResponseBuilder::buildValidationErrorResponse("delete_sector", "id", "", "required");
    }

    const int id = args.value("id").toInt(-1);
    GeometryResult result = m_geometryService->deleteSector(id);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse("delete_sector", result.errorCode, result.message);
    }
    return JsonResponseBuilder::buildSuccessResponse("delete_sector", QJsonObject{{"deleted_id", id}});
}

QByteArray GeometryCommandHandler::listShapes(const QJsonObject& /*args*/)
{
    return JsonResponseBuilder::buildSuccessResponse("list_shapes", m_geometryService->listShapes());
}
