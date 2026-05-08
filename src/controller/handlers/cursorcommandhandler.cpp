#include "cursorcommandhandler.h"

#include "../json/jsonresponsebuilder.h"
#include "../json/jsonserializer.h"
#include "../json/validators/jsonvalidator.h"
#include "../services/cursorservice.h"
#include "../services/obmservice.h"
#include "commandContext.h"
#include "network/iTransport.h"

#include <QDebug>

CursorCommandHandler::CursorCommandHandler(CommandContext* context, ITransport* transport, ObmService* obmService)
    : m_context(context), m_transport(transport), m_obmService(obmService), m_cursorService(std::make_unique<CursorService>(context))
{
    Q_ASSERT(m_context);
    Q_ASSERT(m_transport);
    Q_ASSERT(m_obmService);
}

QByteArray CursorCommandHandler::createLine(const QJsonObject& args)
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

    CursorCreateRequest request;
    const auto obmPosition = m_obmService->getCurrentPosition();
    request.azimut = JsonValidator::getNumericValue(args, "azimut");
    request.length = JsonValidator::getNumericValue(args, "length");
    request.type = JsonValidator::getIntValue(args, "type", 0);
    request.x = obmPosition.first;
    request.y = obmPosition.second;

    CursorOperationResult result = m_cursorService->createCursor(request);
    if (!result.success) {
        qWarning() << "[CursorCommandHandler] Error al crear linea:" << result.message;
        return JsonResponseBuilder::buildErrorResponse(
            "create_line",
            result.errorCode,
            result.message
        );
    }

    return buildLineCreatedSuccessResponse(result.lineId);
}

QByteArray CursorCommandHandler::deleteLine(const QJsonObject& args)
{
    ValidationResult idValidation = JsonValidator::validateStringField(args, "id", true);
    if (!idValidation.isValid) {
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            idValidation.errorCode,
            idValidation.errorMessage
        );
    }

    const QString lineId = args.value("id").toString();

    ValidationResult formatValidation = JsonValidator::validateLineIdFormat(lineId);
    if (!formatValidation.isValid) {
        QJsonObject details;
        details["id"] = lineId;
        details["expected_format"] = "LINE_<numero>";
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            formatValidation.errorCode,
            formatValidation.errorMessage,
            details
        );
    }

    bool ok = false;
    const int cursorId = CursorService::cursorIdFromLineId(lineId, &ok);
    if (!ok) {
        QJsonObject details;
        details["id"] = lineId;
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            "INVALID_ID_FORMAT",
            QString("No se pudo extraer numero del ID: %1").arg(lineId),
            details
        );
    }

    CursorOperationResult result = m_cursorService->deleteCursorById(cursorId);
    if (!result.success) {
        QJsonObject details;
        details["id"] = lineId;
        return JsonResponseBuilder::buildErrorResponse(
            "delete_line",
            "LINE_NOT_FOUND",
            QString("Linea no encontrada: %1").arg(lineId),
            details
        );
    }

    return buildLineDeletedSuccessResponse(lineId);
}

QByteArray CursorCommandHandler::listLines(const QJsonObject&)
{
    return buildLineListSuccessResponse();
}

QByteArray CursorCommandHandler::buildLineCreatedSuccessResponse(const QString& lineId)
{
    const QJsonArray linesArray = JsonSerializer::serializeLineList(m_context->getCursors());
    qDebug() << "[CursorCommandHandler] Respuesta create_line con" << linesArray.size() << "lineas";
    return JsonResponseBuilder::buildLineCreatedResponse("create_line", lineId, linesArray);
}

QByteArray CursorCommandHandler::buildLineDeletedSuccessResponse(const QString& lineId)
{
    const QJsonArray linesArray = JsonSerializer::serializeLineList(m_context->getCursors());
    qDebug() << "[CursorCommandHandler] Respuesta delete_line con" << linesArray.size() << "lineas";
    return JsonResponseBuilder::buildLineDeletedResponse("delete_line", lineId, linesArray);
}

QByteArray CursorCommandHandler::buildLineListSuccessResponse() const
{
    const QJsonArray linesArray = JsonSerializer::serializeLineList(m_context->getCursors());
    qDebug() << "[CursorCommandHandler] Respuesta list_lines con" << linesArray.size() << "lineas";
    return JsonResponseBuilder::buildLineListResponse("list_lines", linesArray);
}
