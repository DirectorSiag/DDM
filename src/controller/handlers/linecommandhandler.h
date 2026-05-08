#pragma once

#include <QJsonObject>
#include <QByteArray>
#include <QString>

class CommandContext;
class ITransport;

/**
 * @brief Manejador de comandos de líneas separado por responsabilidad
 */
class LineCommandHandler
{
public:
    LineCommandHandler(CommandContext* context, ITransport* transport);
    
    /**
     * @brief Crea una nueva línea en el sistema
     * @return QByteArray con la respuesta JSON a enviar
     */
    QByteArray createLine(const QJsonObject& args);
    
    /**
     * @brief Elimina una línea existente del sistema
     * @return QByteArray con la respuesta JSON a enviar
     */
    QByteArray deleteLine(const QJsonObject& args);
    
private:
    CommandContext* m_context;
    ITransport* m_transport;
    
    QString generateLineId();
    QByteArray buildLineCreatedSuccessResponse(const QString& lineId);
    
    int extractCursorIdFromLineId(const QString& lineId, bool* ok);
    QByteArray buildLineDeletedSuccessResponse(const QString& lineId);
};
