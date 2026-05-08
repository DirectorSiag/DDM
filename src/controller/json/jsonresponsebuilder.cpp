#include "jsonresponsebuilder.h"
#include <QJsonDocument>
#include <QDebug>

QByteArray JsonResponseBuilder::buildSuccessResponse(
    const QString& command,
    const QJsonObject& args)
{
    QJsonObject response;
    response["status"] = "success";
    response["command"] = command;
    response["args"] = args;
    
    QJsonDocument doc(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "[JsonResponseBuilder] Respuesta exitosa construida:" << jsonData;
    
    return jsonData;
}

QByteArray JsonResponseBuilder::buildLineCreatedResponse(
    const QString& command,
    const QString& createdId,
    const QJsonArray& allLines)
{
    QJsonObject args;
    args["created_id"] = createdId;
    args["lines"] = allLines;
    
    return buildSuccessResponse(command, args);
}

QByteArray JsonResponseBuilder::buildLineDeletedResponse(
    const QString& command,
    const QString& deletedId,
    const QJsonArray& remainingLines)
{
    QJsonObject args;
    args["deleted_id"] = deletedId;
    args["lines"] = remainingLines;
    
    return buildSuccessResponse(command, args);
}

QByteArray JsonResponseBuilder::buildLineListResponse(
    const QString& command,
    const QJsonArray& allLines)
{
    QJsonObject args;
    args["lines"] = allLines;
    
    return buildSuccessResponse(command, args);
}

QByteArray JsonResponseBuilder::buildErrorResponse(
    const QString& command,
    const QString& errorCode,
    const QString& errorMessage,
    const QJsonObject& details)
{
    QJsonObject response;
    response["status"] = "error";
    response["command"] = command;
    
    QJsonObject argsObj;
    argsObj["code"] = errorCode;
    argsObj["message"] = errorMessage;
    
    if (!details.isEmpty()) {
        argsObj["details"] = details;
    }
    
    response["args"] = argsObj;
    
    QJsonDocument doc(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qWarning() << "[JsonResponseBuilder] Respuesta de error construida:" << jsonData;
    
    return jsonData;
}

QByteArray JsonResponseBuilder::buildValidationErrorResponse(
    const QString& command,
    const QString& fieldName,
    const QString& fieldValue,
    const QString& expectedRange)
{
    QJsonObject details;
    details["field"] = fieldName;
    details["value"] = fieldValue;
    details["expected"] = expectedRange;
    
    QString errorCode = QString("INVALID_%1").arg(fieldName.toUpper());
    QString errorMessage = QString("%1 %2 fuera de rango [%3]")
        .arg(fieldName)
        .arg(fieldValue)
        .arg(expectedRange);
    
    return buildErrorResponse(command, errorCode, errorMessage, details);
}
