#include "jsoncommandhandler.h"
#include "../handlers/linecommandhandler.h"
#include "jsonresponsebuilder.h"
#include "commandContext.h"
#include "network/iTransport.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

JsonCommandHandler::JsonCommandHandler(CommandContext* context, ITransport* transport, QObject *parent)
    : QObject(parent), m_transport(transport){
    Q_ASSERT(context);
    Q_ASSERT(m_transport);
    
    // Crea un puntero único a LineCommandHandler y se asegura ownership
    m_lineHandler = std::make_unique<LineCommandHandler>(context, transport);
    
    // Inicializar el mapa de comandos
    initializeCommandMap();
}

JsonCommandHandler::~JsonCommandHandler() = default;

void JsonCommandHandler::processJsonCommand(const QByteArray& jsonData){   
    QJsonObject obj;
    // En obj obtenemos el resultado del parseo
    if (!parseJson(jsonData, obj))
        return;
    
    QString command = obj.value("command").toString();
    QJsonObject args = obj.value("args").toObject();
    routeCommand(command, args);
}

bool JsonCommandHandler::parseJson(const QByteArray& jsonData, QJsonObject& outObject){
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        sendParseError(parseError.errorString());
        return false;
    }
    
    if (!doc.isObject()) {
        sendParseError("El JSON debe ser un objeto");
        return false;
    }
    
    outObject = doc.object();
    return true;
}

void JsonCommandHandler::initializeCommandMap()
{
    // Registrar comandos de línea
    m_commandMap["create_line"] = [this](const QJsonObject& args) {
        return m_lineHandler->createLine(args);
    };
    
    m_commandMap["delete_line"] = [this](const QJsonObject& args) {
        return m_lineHandler->deleteLine(args);
    };

    // Agregar nuevos comandos ACA.
    //URI ACA HAY QUE REMAPEAR PARA EMPEZAR LOS CPA
}

void JsonCommandHandler::routeCommand(const QString& command, const QJsonObject& args)
{
    auto it = m_commandMap.find(command);
    
    if (it != m_commandMap.end()) {
        QByteArray response = it.value()(args);
        sendResponse(response);
    } else {
        sendUnknownCommandError(command);
    }
}

void JsonCommandHandler::sendResponse(const QByteArray& responseData)
{
    if (!m_transport) {
        qWarning() << "[JsonCommandHandler] Transport no disponible";
        return;
    }
    
    if (!m_transport->send(responseData)) {
        qWarning() << "[JsonCommandHandler] Fallo al enviar respuesta";
    }
}

void JsonCommandHandler::sendParseError(const QString& errorDetail)
{
    qWarning() << "[JsonCommandHandler] Error de parseo JSON:" << errorDetail;
    QByteArray errorResponse = JsonResponseBuilder::buildErrorResponse(
        "unknown",
        "INVALID_JSON",
        QString("Error al parsear JSON: %1").arg(errorDetail)
    );
    sendResponse(errorResponse);
}

void JsonCommandHandler::sendUnknownCommandError(const QString& command)
{
    qWarning() << "[JsonCommandHandler] Comando no reconocido:" << command;
    QByteArray errorResponse = JsonResponseBuilder::buildErrorResponse(
        command,
        "UNKNOWN_COMMAND",
        QString("Comando no reconocido: %1").arg(command)
    );
    sendResponse(errorResponse);
}

// void JsonCommandHandler::handleStartCPA(QJsonObject args){
//     // 1. Extraer argumentos requeridos
//     QJsonValue cpa_id_val = args.value("id");
//     QJsonValue TN_Source_val = args.value("track-a");
//     QJsonValue TN_Target_val = args.value("track-b");

//     int cpa_id = cpa_id_val.toInt();
//     int TN_Source = TN_Source_val.toInt();
//     int TN_Target = TN_Target_val.toInt();
//     Track *trackA = m_context->findTrackById(TN_Source);
//     Track *trackB = m_context->findTrackById(TN_Target);

//     QPair<double, double> start_posA = {trackA->getX(), trackA->getY()};
//     QPair<double, double> start_posB = {trackB->getX(), trackB->getY()};

//     QElapsedTimer elapsedTime;

//     QTimer *start_timer = new QTimerEvent(this);
//     connect(start_timer, qOverload<>(&QTimer::start),
//             this, [this]() {

//         //QPair<double, double> start_posA = {trackA->getX(), trackA->getY()};
//         //QPair<double, double> start_posB = {trackB->getX(), trackB->getY()};
//         double timeA;
//         double timeB;

//         //tiene que pasar un tiempo
//         QPair<double, double> final_posA = {trackA->getX(), trackA->getY()};
//         QPair<double, double> final_posB = {trackB->getX(), trackB->getY()};

//         QPair<double, double> deltaA = (final_posA - start_posA);
//         QPair<double, double> deltaB = (final_posB - start_posB);

//         QPair<double, double> velocityA =
//             deltaA / timeInterval;

//         QPair<double, double> velocityB =
//             deltaB / timeInterval;

//         QPair<QPair<double,double>, double> rPol_a = {deltaA, timeA};
//         //         |   (x , y)    | time |

//         QPair<QPair<double,double>, double> rPol_b = {deltaB, timeB};
//         //         |   (x , y)    | time |


//         double timeCPA = (deltaB - deltaA)/ (velocityA - velocityB);

//         int timeInterval = start_timer->interval()


//         QPair<double, double> posInTimeA = deltaA + velocityA * timeInterval;
//         QPair<double, double> posInTimeB = deltaB + velocityB * timeInterval;

//         QPair<double, double> cpaInCurrentTime =
//             (final_posA - final_posB) / (velocityA-velocityB);


//     });

//     start_timer->start(5000);
//     elapsedTime.start();

// }
