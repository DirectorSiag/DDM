#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <memory>
#include "../services/geometryservice.h"

class CommandContext;
class ITransport;
class ObmService;

class GeometryCommandHandler
{
public:
    GeometryCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService);

    QByteArray createArea(const QJsonObject& args);
    QByteArray deleteArea(const QJsonObject& args);
    QByteArray createCircle(const QJsonObject& args);
    QByteArray deleteCircle(const QJsonObject& args);
    QByteArray createPolygon(const QJsonObject& args);
    QByteArray deletePolygon(const QJsonObject& args);
    QByteArray listShapes(const QJsonObject& args);

private:
    CommandContext* m_context;
    ITransport* m_transport;
    ObmService* m_obmService;
    std::unique_ptr<GeometryService> m_geometryService;
};
