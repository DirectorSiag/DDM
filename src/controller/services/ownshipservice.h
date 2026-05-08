#pragma once

#include <QJsonObject>
#include <QString>

class CommandContext;

struct OwnShipOperationResult {
    bool success = false;
    QString errorCode;
    QString message;
};

class OwnShipService
{
public:
    explicit OwnShipService(CommandContext* context);

    OwnShipOperationResult updateFromJson(const QJsonObject& args);
    OwnShipOperationResult setFromCli(double courseDeg,
                                      double speedKnots,
                                      const QString& source = QStringLiteral("CLI"));

    QJsonObject serializeOwnShip() const;
    QString formatOwnShip() const;

private:
    static double normalize360(double deg);
    void syncOwnShipVirtualTrack();

    CommandContext* m_context;
};
