#include "dsiCommand.h"
#include "../services/dsiService.h"
#include "model/utils/RadarMath.h"

namespace {

bool parseGmsTriplet(const QString& val, int& deg, int& min, double& sec)
{
    const QStringList parts = val.split(',');
    if (parts.size() != 3) return false;

    bool ok = true, okTmp;
    deg = parts[0].trimmed().toInt(&okTmp); ok &= okTmp;
    min = parts[1].trimmed().toInt(&okTmp); ok &= okTmp;
    sec = parts[2].trimmed().toDouble(&okTmp); ok &= okTmp;
    return ok;
}

}

CommandResult DSICommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return { false, QStringLiteral("Faltan argumentos.\n%1").arg(usage()) };
    }

    QMap<QString, QString> opts;
    for (const QString& token : inv.args) {
        const int eq = token.indexOf('=');
        if (eq > 0) {
            QString key = token.left(eq);
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), token.mid(eq + 1).trimmed());
        } else if (token.startsWith('-')) {
            QString key = token;
            while (key.startsWith('-')) key.remove(0, 1);
            opts.insert(key.toLower(), QStringLiteral("true"));
        }
    }

    DSIService service(&ctx);

    // dsi --list
    if (opts.contains(QStringLiteral("list"))) {
        const DSIOperationResult r = service.listDSI();
        return { r.ok, r.message };
    }

    // dsi --info=<tn>
    if (opts.contains(QStringLiteral("info"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("info")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--info requiere un TN numerico.") };
        const DSIOperationResult r = service.infoDSI(tn);
        return { r.ok, r.message };
    }

    // dsi --borrar=<tn>
    if (opts.contains(QStringLiteral("borrar"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("borrar")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--borrar requiere un TN numerico.") };
        const DSIOperationResult r = service.deleteDSI(tn);
        return { r.ok, r.message };
    }

    // dsi --editar=<tn> [--texto=<str>] [--radio=<mn>] [--estilo=<e>]
    //                   [--color=<c>] [--fondo=<c>]
    if (opts.contains(QStringLiteral("editar"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("editar")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--editar requiere un TN numerico.") };

        const DSIOperationResult r = service.editDSI(tn, opts);
        return { r.ok, r.message };
    }

    // dsi --altrack=<trackId> --tn=<tn>
    if (opts.contains(QStringLiteral("altrack")) && opts.contains(QStringLiteral("tn"))) {
        bool okTrack = false, okTn = false;
        const int trackId = opts.value(QStringLiteral("altrack")).toInt(&okTrack);
        const int tn       = opts.value(QStringLiteral("tn")).toInt(&okTn);
        if (!okTrack || !okTn)
            return { false, QStringLiteral("--altrack y --tn deben ser numericos.") };
        const DSIOperationResult r = service.associateTrack(tn, trackId);
        return { r.ok, r.message };
    }

    // dsi --deltrack --tn=<tn>
    if (opts.contains(QStringLiteral("deltrack")) && opts.contains(QStringLiteral("tn"))
        && !opts.contains(QStringLiteral("nuevo"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("tn")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--tn debe ser numerico.") };
        const DSIOperationResult r = service.dissociateTrack(tn);
        return { r.ok, r.message };
    }

    // dsi --nuevo ...
    if (opts.contains(QStringLiteral("nuevo"))) {

        DSICreateParams params;
        params.labelText = opts.value(QStringLiteral("texto")); // vacío -> default "DSI <radio>MN"

        if (opts.contains(QStringLiteral("radio"))) {
            bool ok = false;
            params.radiusDm = opts.value(QStringLiteral("radio")).toDouble(&ok);
            if (!ok || params.radiusDm <= 0.0)
                return { false, QStringLiteral("--radio debe ser un numero mayor a 0.") };
        }

        if (opts.contains(QStringLiteral("estilo"))) {
            bool ok = false;
            params.lineStyle = DSIService::stringToLineStyle(opts.value(QStringLiteral("estilo")), ok);
            if (!ok) return { false, QStringLiteral("--estilo invalido.") };
        }

        if (opts.contains(QStringLiteral("color"))) {
            bool ok = false;
            params.labelColor = DSIService::stringToColor(opts.value(QStringLiteral("color")), ok);
            if (!ok) return { false, QStringLiteral("--color invalido.") };
            params.hasLabelColor = true;
        }
        if (opts.contains(QStringLiteral("fondo"))) {
            bool ok = false;
            params.labelBackground = DSIService::stringToColor(opts.value(QStringLiteral("fondo")), ok);
            if (!ok) return { false, QStringLiteral("--fondo invalido.") };
            params.hasLabelBackground = true;
        }

        int methodCount = 0;
        if (opts.contains(QStringLiteral("man"))) methodCount++;
        if (opts.contains(QStringLiteral("az")))  methodCount++;
        if (opts.contains(QStringLiteral("lat"))) methodCount++;

        if (methodCount == 0)
            return { false, QStringLiteral("Debe especificar un metodo de posicion: --man, --az/--dt o --lat/--lon.") };
        if (methodCount > 1)
            return { false, QStringLiteral("Solo se permite un metodo de posicion a la vez (REQ-DSI-UI-015).") };

        if (opts.contains(QStringLiteral("man"))) {
            const QStringList parts = opts.value(QStringLiteral("man")).split(',');
            if (parts.size() != 2)
                return { false, QStringLiteral("--man requiere formato <xDm>,<yDm>.") };
            bool okX = false, okY = false;
            const double x = parts[0].trimmed().toDouble(&okX);
            const double y = parts[1].trimmed().toDouble(&okY);
            if (!okX || !okY)
                return { false, QStringLiteral("--man: los valores deben ser numericos.") };
            params.positionMethod = DSIPositionMethod::Manual;
            params.manXDm = x;
            params.manYDm = y;
        }
        else if (opts.contains(QStringLiteral("az"))) {
            if (!opts.contains(QStringLiteral("dt")))
                return { false, QStringLiteral("--az requiere --dt=<distancia_mn>.") };
            if (!opts.contains(QStringLiteral("v")) && !opts.contains(QStringLiteral("r")))
                return { false, QStringLiteral("--az requiere --v (Verdadero) o --r (Relativo).") };
            if (opts.contains(QStringLiteral("v")) && opts.contains(QStringLiteral("r")))
                return { false, QStringLiteral("--v y --r son mutuamente excluyentes.") };

            bool okAz = false, okDt = false;
            const double az = opts.value(QStringLiteral("az")).toDouble(&okAz);
            const double dt = opts.value(QStringLiteral("dt")).toDouble(&okDt);
            if (!okAz || !okDt)
                return { false, QStringLiteral("--az y --dt deben ser numericos.") };

            params.positionMethod = DSIPositionMethod::AzimutDist;
            params.azimuthDeg   = az;
            params.distanceDm   = dt;
            params.useVerdadero = opts.contains(QStringLiteral("v"));
        }
        else if (opts.contains(QStringLiteral("lat"))) {
            if (!opts.contains(QStringLiteral("lon")))
                return { false, QStringLiteral("--lat requiere --lon=<g,m,s>.") };

            int latDeg, latMin, lonDeg, lonMin;
            double latSec, lonSec;
            if (!parseGmsTriplet(opts.value(QStringLiteral("lat")), latDeg, latMin, latSec) ||
                !parseGmsTriplet(opts.value(QStringLiteral("lon")), lonDeg, lonMin, lonSec)) {
                return { false, QStringLiteral("--lat y --lon deben tener formato <grados>,<minutos>,<segundos>.") };
            }

            params.positionMethod = DSIPositionMethod::LatLon;
            params.latDecimal = RadarMath::dmsToDecimal(latDeg, latMin, latSec);
            params.lonDecimal = RadarMath::dmsToDecimal(lonDeg, lonMin, lonSec);
        }

        if (opts.contains(QStringLiteral("altrack"))) {
            bool ok = false;
            const int trackId = opts.value(QStringLiteral("altrack")).toInt(&ok);
            if (!ok) return { false, QStringLiteral("--altrack requiere un ID de track numerico.") };
            params.asociarAlCrear = true;
            params.trackToAssociate = trackId;
        }

        const DSIOperationResult r = service.createDSI(params);
        return { r.ok, r.message };
    }

    return { false, QStringLiteral("Flag no reconocido.\n%1").arg(usage()) };
}
