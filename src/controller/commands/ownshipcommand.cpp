#include "ownshipcommand.h"
#include "../services/ownshipservice.h"
#include "model/utils/RadarMath.h"
#include <optional>

namespace {
bool parseDoubleArg(const QString& text, double& out)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok) return false;
    out = value;
    return true;
}

bool parseDms(const QString& val, double& outDecimal)
{
    const QStringList parts = val.split(',');
    if (parts.size() != 3) return false;

    bool ok = true, okTmp;
    const int    deg = parts[0].trimmed().toInt(&okTmp);   ok &= okTmp;
    const int    min = parts[1].trimmed().toInt(&okTmp);   ok &= okTmp;
    const double sec = parts[2].trimmed().toDouble(&okTmp); ok &= okTmp;

    if (!ok) return false;

    outDecimal = RadarMath::dmsToDecimal(deg, min, sec);
    return true;
}
}

CommandResult OwnShipCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    OwnShipService ownShipService(&ctx);

    if (inv.args.isEmpty() ||
        inv.args.first().compare(QStringLiteral("show"), Qt::CaseInsensitive) == 0) {
        return { true, ownShipService.formatOwnShip() };
    }

    const QString action = inv.args.first().toLower();

    if (action == QStringLiteral("setgeodms")) {
        if (inv.args.size() < 7) {
            return {false, QStringLiteral("Faltan argumentos. Uso: ownship setgeodms <latDeg> <latMin> <latSec> <lonDeg> <lonMin> <lonSec>")};
        }
        bool ok = true; bool okTmp;
        const int    latDeg = inv.args[1].toInt(&okTmp);    ok &= okTmp;
        const int    latMin = inv.args[2].toInt(&okTmp);    ok &= okTmp;
        const double latSec = inv.args[3].toDouble(&okTmp); ok &= okTmp;
        const int    lonDeg = inv.args[4].toInt(&okTmp);    ok &= okTmp;
        const int    lonMin = inv.args[5].toInt(&okTmp);    ok &= okTmp;
        const double lonSec = inv.args[6].toDouble(&okTmp); ok &= okTmp;
        if (!ok) {
            return {false, QStringLiteral("Argumentos numericos invalidos.")};
        }
        const OwnShipOperationResult result = ownShipService.setGeoFromCliDms(
            latDeg, latMin, latSec, lonDeg, lonMin, lonSec);
        if (!result.success) {
            return {false, result.message};
        }
        return {true, ownShipService.formatOwnShip()};
    }

    if (action == QStringLiteral("setgeo")) {
        if (inv.args.size() < 3) {
            return {false, QStringLiteral("Faltan argumentos. Uso: %1").arg(usage())};
        }
        double latDeg = 0.0;
        double lonDeg = 0.0;
        if (!parseDoubleArg(inv.args[1], latDeg)
            || !parseDoubleArg(inv.args[2], lonDeg)) {
            return {false, QStringLiteral("Argumentos numericos invalidos. Uso: %1").arg(usage())};
        }
        const OwnShipOperationResult result = ownShipService.setGeoFromCli(latDeg, lonDeg);
        if (!result.success) {
            return {false, result.message};
        }
        return {true, ownShipService.formatOwnShip()};
    }

    if (action != QStringLiteral("set")) {
        return { false, QStringLiteral("Accion invalida. Uso: %1").arg(usage()) };
    }

    if (inv.args.size() < 3) {
        return { false, QStringLiteral("Faltan argumentos. Uso: %1").arg(usage()) };
    }

    double courseDeg = 0.0;
    double speedKnots = 0.0;

    if (!parseDoubleArg(inv.args[1], courseDeg) ||
        !parseDoubleArg(inv.args[2], speedKnots)) {
        return { false, QStringLiteral("Argumentos numericos invalidos. Uso: %1").arg(usage()) };
    }

    // Source opcional (tercer arg posicional, no empieza con --)
    QString source = QStringLiteral("CLI");
    int nextArgIdx = 3;
    if (inv.args.size() > 3 && !inv.args[3].startsWith(QStringLiteral("--"))) {
        source = inv.args[3];
        nextArgIdx = 4;
    }

    // Parsear flags opcionales --lat y --lon
    QMap<QString, QString> opts;
    for (int i = nextArgIdx; i < inv.args.size(); ++i) {
        const QString& token = inv.args[i];
        const int eq = token.indexOf('=');
        if (eq > 0) {
            QString key = token.left(eq);
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), token.mid(eq + 1).trimmed());
        }
    }

    // Lat/lon opcionales en GMS
    std::optional<double> latDecimal;
    std::optional<double> lonDecimal;

    if (opts.contains(QStringLiteral("lat"))) {
        double val = 0.0;
        if (!parseDms(opts.value(QStringLiteral("lat")), val)) {
            return { false, QStringLiteral("--lat invalido. Formato esperado: <grados>,<minutos>,<segundos>.") };
        }
        if (val < -90.0 || val > 90.0) {
            return { false, QStringLiteral("--lat debe estar en el rango [-90, 90].") };
        }
        latDecimal = val;
    }

    if (opts.contains(QStringLiteral("lon"))) {
        double val = 0.0;
        if (!parseDms(opts.value(QStringLiteral("lon")), val)) {
            return { false, QStringLiteral("--lon invalido. Formato esperado: <grados>,<minutos>,<segundos>.") };
        }
        if (val < -180.0 || val > 180.0) {
            return { false, QStringLiteral("--lon debe estar en el rango [-180, 180].") };
        }
        lonDecimal = val;
    }

    const OwnShipOperationResult result = ownShipService.setFromCli(
        courseDeg,
        speedKnots,
        source,
        latDecimal,
        lonDecimal
        );

    if (!result.success) {
        return { false, result.message };
    }

    return { true, ownShipService.formatOwnShip() };
}
