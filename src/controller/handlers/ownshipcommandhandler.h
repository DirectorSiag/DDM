#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <memory>

class CommandContext;
class OwnShipService;

class OwnShipCommandHandler
{
public:
    explicit OwnShipCommandHandler(CommandContext* context);
    ~OwnShipCommandHandler();

    QByteArray updateOwnShip(const QJsonObject& args);

private:
    std::unique_ptr<OwnShipService> m_ownShipService;
};
