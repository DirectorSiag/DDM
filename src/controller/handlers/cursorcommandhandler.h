#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <memory>
#include "../services/cursorservice.h"

class CommandContext;
class ITransport;
class ObmService;

class CursorCommandHandler
{
public:
    CursorCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService);

    QByteArray createLine(const QJsonObject& args);
    QByteArray deleteLine(const QJsonObject& args);
    QByteArray listLines(const QJsonObject& args);

private:
    CommandContext* m_context;
    ITransport* m_transport;
    ObmService* m_obmService;
    std::unique_ptr<CursorService> m_cursorService;

    QByteArray buildLineCreatedSuccessResponse(const QString& lineId);
    QByteArray buildLineDeletedSuccessResponse(const QString& lineId);
    QByteArray buildLineListSuccessResponse() const;
};
