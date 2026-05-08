#include "ownshipcommand.h"

#include "../services/ownshipservice.h"

namespace {
bool parseDoubleArg(const QString& text, double& out)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok) {
        return false;
    }
    out = value;
    return true;
}
}

CommandResult OwnShipCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    OwnShipService ownShipService(&ctx);

    if (inv.args.isEmpty() || inv.args.first().compare(QStringLiteral("show"), Qt::CaseInsensitive) == 0) {
        return {true, ownShipService.formatOwnShip()};
    }

    const QString action = inv.args.first().toLower();
    if (action != QStringLiteral("set")) {
        return {false, QStringLiteral("Accion invalida. Uso: %1").arg(usage())};
    }

    if (inv.args.size() < 3) {
        return {false, QStringLiteral("Faltan argumentos. Uso: %1").arg(usage())};
    }

    double courseDeg = 0.0;
    double speedKnots = 0.0;

    if (!parseDoubleArg(inv.args[1], courseDeg)
        || !parseDoubleArg(inv.args[2], speedKnots)) {
        return {false, QStringLiteral("Argumentos numericos invalidos. Uso: %1").arg(usage())};
    }

    const QString source = (inv.args.size() > 3) ? inv.args[3] : QStringLiteral("CLI");

    const OwnShipOperationResult result = ownShipService.setFromCli(
        courseDeg,
        speedKnots,
        source
    );

    if (!result.success) {
        return {false, result.message};
    }

    return {true, ownShipService.formatOwnShip()};
}
