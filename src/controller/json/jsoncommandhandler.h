#pragma once

#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <memory>
#include <functional>
#include <QMap>

/*
 * Este módulo se encarga de parsear los comandos json recibidos desde el frontend
 * y delegar a los handlers especializados según el tipo de comando.
 * */

class CommandContext;
class ITransport;
class LineCommandHandler;

class JsonCommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit JsonCommandHandler(CommandContext* context, ITransport* transport, QObject *parent = nullptr);
    ~JsonCommandHandler();

public slots:
    void processJsonCommand(const QByteArray& jsonData);

private:
    // Declaración de tipo para manejadores de comandos
    using CommandHandler = std::function<QByteArray(const QJsonObject&)>;
    
    ITransport* m_transport;
    std::unique_ptr<LineCommandHandler> m_lineHandler;
    QMap<QString, CommandHandler> m_commandMap;
    
    void initializeCommandMap();
    bool parseJson(const QByteArray& jsonData, QJsonObject& outObject);
    void routeCommand(const QString& command, const QJsonObject& args);
    void sendResponse(const QByteArray& responseData);
    void sendParseError(const QString& errorDetail);
    void sendUnknownCommandError(const QString& command);
    void handleStartCPA(QJsonObject);
};
