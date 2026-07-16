#pragma once

#include <QObject>
#include <QByteArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QString>
#include <memory>
#include <functional>
#include <QMap>

/*
 * Este módulo se encarga de parsear los comandos json recibidos desde el frontend
 * y delegar a los handlers especializados según el tipo de comando.
 * */

class CommandContext;
class ITransport;
class CursorCommandHandler;
class GeometryCommandHandler;
class TrackCommandHandler;
class OwnShipCommandHandler;
class ObmService;
class CPAService;
class EstacionamientoService;
class FondeoService;
class TwoWService;
struct CPATrackRef;

class JsonCommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit JsonCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService, QObject *parent = nullptr);
    ~JsonCommandHandler();

public slots:
    void processJsonCommand(const QByteArray& jsonData);
    void refreshActiveCpaSessions();

private:
    // Declaración de tipo para manejadores de comandos
    using CommandHandler = std::function<QByteArray(const QJsonObject&)>;
    
    ITransport* m_transport;
    CommandContext* m_context;
    ObmService* m_obmService;
    std::unique_ptr<TrackCommandHandler> m_trackHandler;
    std::unique_ptr<OwnShipCommandHandler> m_ownShipHandler;
    std::unique_ptr<CursorCommandHandler> m_cursorHandler;
    std::unique_ptr<GeometryCommandHandler> m_geometryHandler;
    std::unique_ptr<CPAService> m_cpaService;
    std::unique_ptr<EstacionamientoService> m_estacionamientoService;
    std::unique_ptr<FondeoService> m_fondeoService;
    std::unique_ptr<TwoWService> m_twoWService;
    QMap<int, QString> m_cpaSlotSessions;
    QMap<QString, CommandHandler> m_commandMap;
    
    void initializeCommandMap();
    bool parseJson(const QByteArray& jsonData, QJsonObject& outObject);
    void routeCommand(const QString& command, const QJsonObject& args);
    void sendResponse(const QByteArray& responseData);
    void sendParseError(const QString& errorDetail);
    void sendUnknownCommandError(const QString& command);

    QByteArray handleCpaStart(const QJsonObject& args);
    QByteArray handleEstacionamiento(const QJsonObject& args);
    QByteArray handleEstacionamientoStop(const QJsonObject& args);
    QByteArray handleFondeoStart(const QJsonObject& args);
    QByteArray handleFondeoStop(const QJsonObject& args);
    QByteArray handleFondeoInfo(const QJsonObject& args);
    QByteArray handleFondeoTipos(const QJsonObject& args);
    QByteArray handleTwoWStart(const QJsonObject& args);
    QByteArray handleTwoWStop(const QJsonObject& args);
    QByteArray handleTwoWInfo(const QJsonObject& args);
    QByteArray handleTwoWSetStations(const QJsonObject& args);
    QByteArray handlePppGraph(const QJsonObject& args);
    QByteArray handlePppFinish(const QJsonObject& args);
    QByteArray handlePppClearTrack(const QJsonObject& args);

    bool parseTrackRefValue(const QJsonValue& value, CPATrackRef& outRef, QString& errorReason) const;
    bool parseTrackPair(const QJsonObject& args, CPATrackRef& trackA, CPATrackRef& trackB, QString& errorField, QString& errorReason) const;
};
