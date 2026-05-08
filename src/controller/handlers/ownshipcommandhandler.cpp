#include "ownshipcommandhandler.h"

#include "../json/jsonresponsebuilder.h"
#include "../services/ownshipservice.h"

OwnShipCommandHandler::OwnShipCommandHandler(CommandContext* context)
    : m_ownShipService(std::make_unique<OwnShipService>(context))
{
}

OwnShipCommandHandler::~OwnShipCommandHandler() = default;

QByteArray OwnShipCommandHandler::updateOwnShip(const QJsonObject& args)
{
    const OwnShipOperationResult result = m_ownShipService->updateFromJson(args);
    if (!result.success) {
        return JsonResponseBuilder::buildErrorResponse(
            QStringLiteral("ownship_update"),
            result.errorCode,
            result.message
        );
    }

    QJsonObject responseArgs;
    responseArgs["updated"] = true;
    responseArgs["ownship"] = m_ownShipService->serializeOwnShip();
    return JsonResponseBuilder::buildSuccessResponse(QStringLiteral("ownship_update"), responseArgs);
}
