#include "linecommandhandler.h"
#include "../json/validators/jsonvalidator.h"
#include "../json/jsonresponsebuilder.h"
#include "../json/jsonserializer.h"
#include "commandContext.h"
#include "entities/cursorEntity.h"
#include "network/iTransport.h"
#include <QDebug>
#include <qfloat16.h>

LineCommandHandler::LineCommandHandler(CommandContext* context, ITransport* transport)
    : m_context(context), m_transport(transport)
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_transport);
}

QByteArray LineCommandHandler::createLine(const QJsonObject& args)
{
    ValidationResult azimutValidation = JsonValidator::validateNumericField(
        args, "azimut", 0.0, 359.9, true
    );
    if (!azimutValidation.isValid) {
        return JsonResponseBuilder::buildValidationErrorResponse(
            "create_line",
            azimutValidation.fieldName,
            azimutValidation.fieldValue,
            azimutValidation.expectedRange
        );
    }
    
    ValidationResult lengthValidation = JsonValidator::validateNumericField(
        args, "length", 0.1, 256.0, true
    );
    if (!lengthValidation.isValid) {
        return JsonResponseBuilder::buildValidationErrorResponse(
            "create_line",
            lengthValidation.fieldName,
            lengthValidation.fieldValue,
            lengthValidation.expectedRange
        );
    }
    
    // Validar campos opcionales
    ValidationResult typeValidation = JsonValidator::validateIntegerField(
        args, "type", 0, 7, false, 0
    );
    if (!typeValidation.isValid) {
        return JsonResponseBuilder::buildValidationErrorResponse(
            "create_line",
            typeValidation.fieldName,
            typeValidation.fieldValue,
            typeValidation.expectedRange
        );
    }
    
    // Extraer valores validados
    double azimut = JsonValidator::getNumericValue(args, "azimut");
    double length = JsonValidator::getNumericValue(args, "length");
    int type = JsonValidator::getIntValue(args, "type", 0);
    double x = JsonValidator::getNumericValue(args, "x", 0.0);
    double y = JsonValidator::getNumericValue(args, "y", 0.0);
    
    QString lineId = generateLineId();
    QPair<qfloat16, qfloat16> coords((qfloat16(x)), qfloat16(y));
    
    try {
        int cursorId = QStringView(lineId).mid(5).toInt(); //Para no tener problemas de Clazy con el mid y el heap
        m_context->emplaceCursorFront(
            coords,
            qfloat16(azimut),
            qfloat16(length),
            type,
            cursorId,
            true
        );
        
        return buildLineCreatedSuccessResponse(lineId);
        
    } catch (const std::exception& e) {
        qWarning() << "[LineCommandHandler] Error al crear línea:" << e.what();
        return JsonResponseBuilder::buildErrorResponse(
            "create_line",
            "BACKEND_ERROR",
            QString("Error interno al crear línea: %1").arg(e.what())
        );
    }
}

QByteArray LineCommandHandler::deleteLine(const QJsonObject& args)
{
    // Validar campo ID
    ValidationResult idValidation = JsonValidator::validateStringField(args, "id", true);
    if (!idValidation.isValid) {
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            idValidation.errorCode,
            idValidation.errorMessage
        );
    }
    
    QString lineId = args.value("id").toString();
    
    // Validar formato del ID
    ValidationResult formatValidation = JsonValidator::validateLineIdFormat(lineId);
    if (!formatValidation.isValid) {
        QJsonObject details;
        details["id"] = lineId;
        details["expected_format"] = "LINE_<número>";
        
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            formatValidation.errorCode,
            formatValidation.errorMessage,
            details
        );
    }
    
    // Extraer cursor ID y eliminar
    bool ok;
    int cursorId = extractCursorIdFromLineId(lineId, &ok);
    
    if (!ok) {
        QJsonObject details;
        details["id"] = lineId;
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            "INVALID_ID_FORMAT",
            QString("No se pudo extraer número del ID: %1").arg(lineId),
            details
        );
    }
    
    qDebug() << "[LineCommandHandler] Eliminando línea ID:" << lineId << "(cursor:" << cursorId << ")";
    
    bool deleted = m_context->eraseCursorById(cursorId);
    
    if (deleted) {
        qInfo() << "[LineCommandHandler] Línea eliminada:" << lineId;
        return buildLineDeletedSuccessResponse(lineId);
    } else {
        qWarning() << "[LineCommandHandler] Línea no encontrada:" << lineId;
        QJsonObject details;
        details["id"] = lineId;
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            "LINE_NOT_FOUND",
            QString("Línea no encontrada: %1").arg(lineId),
            details
        );
    }
}

QString LineCommandHandler::generateLineId()
{
    int cursorId = m_context->nextCursorId++;
    return QString("LINE_%1").arg(cursorId);
}

QByteArray LineCommandHandler::buildLineCreatedSuccessResponse(const QString& lineId)
{
    QJsonArray linesArray = JsonSerializer::serializeLineList(m_context->getCursors());
    qDebug() << "[LineCommandHandler] Respuesta exitosa con" << linesArray.size() << "líneas";
    return JsonResponseBuilder::buildLineCreatedResponse("create_line", lineId, linesArray);
}

int LineCommandHandler::extractCursorIdFromLineId(const QString& lineId, bool* ok)
{
    return lineId.mid(5).toInt(ok);
}

QByteArray LineCommandHandler::buildLineDeletedSuccessResponse(const QString& lineId)
{
    QJsonArray linesArray = JsonSerializer::serializeLineList(m_context->getCursors());
    qDebug() << "[LineCommandHandler] Respuesta exitosa con" << linesArray.size() << "líneas restantes";
    return JsonResponseBuilder::buildLineDeletedResponse("delete_line", lineId, linesArray);
}
