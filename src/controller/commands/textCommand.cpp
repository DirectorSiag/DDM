#include "textCommand.h"
#include "../services/textService.h"
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

TextColor stringToColor(const QString& s, bool& ok)
{
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("rojo"))      return TextColor::Rojo;
    if (key == QStringLiteral("verde"))     return TextColor::Verde;
    if (key == QStringLiteral("azul"))      return TextColor::Azul;
    if (key == QStringLiteral("cian"))      return TextColor::Cian;
    if (key == QStringLiteral("magenta"))   return TextColor::Magenta;
    if (key == QStringLiteral("amarillo"))  return TextColor::Amarillo;
    if (key == QStringLiteral("blanco"))    return TextColor::Blanco;
    if (key == QStringLiteral("purpura"))   return TextColor::Purpura;
    if (key == QStringLiteral("marron"))    return TextColor::MarronAnaranjado;
    ok = false;
    return TextColor::Blanco;
}

TextFontSize stringToFontSize(const QString& s, bool& ok)
{
    ok = true;
    const QString key = s.trimmed().toLower();
    if (key == QStringLiteral("xs")) return TextFontSize::XS;
    if (key == QStringLiteral("md")) return TextFontSize::MD;
    if (key == QStringLiteral("lg")) return TextFontSize::LG;
    ok = false;
    return TextFontSize::MD;
}

}

CommandResult TextCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
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

    TextService service(&ctx);

    // texto --list
    if (opts.contains(QStringLiteral("list"))) {
        const TextOperationResult r = service.listLabels();
        return { r.ok, r.message };
    }

    // texto --info=<tn>
    if (opts.contains(QStringLiteral("info"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("info")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--info requiere un TN numerico.") };
        const TextOperationResult r = service.infoLabel(tn);
        return { r.ok, r.message };
    }

    // texto --borrar=<tn>
    if (opts.contains(QStringLiteral("borrar"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("borrar")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--borrar requiere un TN numerico.") };
        const TextOperationResult r = service.deleteLabel(tn);
        return { r.ok, r.message };
    }

    // texto --editar=<tn> [--texto=<str>]
    if (opts.contains(QStringLiteral("editar"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("editar")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--editar requiere un TN numerico.") };
        const TextOperationResult r = service.editLabel(tn, opts);
        return { r.ok, r.message };
    }

    // texto --altrack=<trackId> --tn=<tn>   (asociar posterior a la creacion)
    if (opts.contains(QStringLiteral("altrack")) && opts.contains(QStringLiteral("tn"))) {
        bool okTrack = false, okTn = false;
        const int trackId = opts.value(QStringLiteral("altrack")).toInt(&okTrack);
        const int tn       = opts.value(QStringLiteral("tn")).toInt(&okTn);
        if (!okTrack || !okTn)
            return { false, QStringLiteral("--altrack y --tn deben ser numericos.") };
        const TextOperationResult r = service.associateTrack(tn, trackId);
        return { r.ok, r.message };
    }

    // texto --deltrack --tn=<tn>   (desasociar posterior a la creacion)
    if (opts.contains(QStringLiteral("deltrack")) && opts.contains(QStringLiteral("tn"))
        && !opts.contains(QStringLiteral("nuevo"))) {
        bool ok = false;
        const int tn = opts.value(QStringLiteral("tn")).toInt(&ok);
        if (!ok) return { false, QStringLiteral("--tn debe ser numerico.") };
        const TextOperationResult r = service.dissociateTrack(tn);
        return { r.ok, r.message };
    }

    // texto --nuevo ...
    if (opts.contains(QStringLiteral("nuevo"))) {

        if (!opts.contains(QStringLiteral("texto")))
            return { false, QStringLiteral("--nuevo requiere --texto=<str>.\n%1").arg(usage()) };

        TextCreateParams params;
        params.texto = opts.value(QStringLiteral("texto"));

        // Tamano (opcional, default MD)
        if (opts.contains(QStringLiteral("tamano"))) {
            bool ok = false;
            params.fontSize = stringToFontSize(opts.value(QStringLiteral("tamano")), ok);
            if (!ok) return { false, QStringLiteral("--tamano invalido. Usar xs, md o lg.") };
        }

        // Color (opcional, default Blanco)
        if (opts.contains(QStringLiteral("color"))) {
            bool ok = false;
            params.fontColor = stringToColor(opts.value(QStringLiteral("color")), ok);
            if (!ok) return { false, QStringLiteral("--color invalido.") };
        }
        if (opts.contains(QStringLiteral("borde"))) {
            bool ok = false;
            params.borderColor = stringToColor(opts.value(QStringLiteral("borde")), ok);
            if (!ok) return { false, QStringLiteral("--borde invalido.") };
        }
        if (opts.contains(QStringLiteral("fondo"))) {
            bool ok = false;
            params.backgroundColor = stringToColor(opts.value(QStringLiteral("fondo")), ok);
            if (!ok) return { false, QStringLiteral("--fondo invalido.") };
        }

        // Sección B — un solo método, mutuamente excluyente
        int methodCount = 0;
        if (opts.contains(QStringLiteral("man")))      methodCount++;
        if (opts.contains(QStringLiteral("az")))       methodCount++;
        if (opts.contains(QStringLiteral("lat")))      methodCount++;
        if (opts.contains(QStringLiteral("deltrack"))) methodCount++;

        if (methodCount == 0)
            return { false, QStringLiteral("Debe especificar un metodo de posicion: --man, --az/--dt, --lat/--lon o --deltrack.") };
        if (methodCount > 1)
            return { false, QStringLiteral("Solo se permite un metodo de posicion a la vez.") };

        if (opts.contains(QStringLiteral("man"))) {
            const QStringList parts = opts.value(QStringLiteral("man")).split(',');
            if (parts.size() != 2)
                return { false, QStringLiteral("--man requiere formato <xDm>,<yDm>.") };
            bool okX = false, okY = false;
            const double x = parts[0].trimmed().toDouble(&okX);
            const double y = parts[1].trimmed().toDouble(&okY);
            if (!okX || !okY)
                return { false, QStringLiteral("--man: los valores deben ser numericos.") };
            params.positionMethod = TextPositionMethod::Manual;
            params.manXDm = x;
            params.manYDm = y;
        }
        else if (opts.contains(QStringLiteral("az"))) {
            if (!opts.contains(QStringLiteral("dt")))
                return { false, QStringLiteral("--az requiere --dt=<distancia_dm>.") };
            if (!opts.contains(QStringLiteral("v")) && !opts.contains(QStringLiteral("r")))
                return { false, QStringLiteral("--az requiere --v (Verdadero) o --r (Relativo).") };
            if (opts.contains(QStringLiteral("v")) && opts.contains(QStringLiteral("r")))
                return { false, QStringLiteral("--v y --r son mutuamente excluyentes.") };

            bool okAz = false, okDt = false;
            const double az = opts.value(QStringLiteral("az")).toDouble(&okAz);
            const double dt = opts.value(QStringLiteral("dt")).toDouble(&okDt);
            if (!okAz || !okDt)
                return { false, QStringLiteral("--az y --dt deben ser numericos.") };

            params.positionMethod = TextPositionMethod::AzimutDist;
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

            params.positionMethod = TextPositionMethod::LatLon;
            params.latDecimal = RadarMath::dmsToDecimal(latDeg, latMin, latSec);
            params.lonDecimal = RadarMath::dmsToDecimal(lonDeg, lonMin, lonSec);
        }
        else if (opts.contains(QStringLiteral("deltrack"))) {
            bool ok = false;
            const int trackId = opts.value(QStringLiteral("deltrack")).toInt(&ok);
            if (!ok) return { false, QStringLiteral("--deltrack requiere un ID de track numerico.") };
            params.positionMethod = TextPositionMethod::DelTrack;
            params.refTrackId = trackId;
        }

        // Sección C — asociación opcional al crear
        if (opts.contains(QStringLiteral("altrack"))) {
            bool ok = false;
            const int trackId = opts.value(QStringLiteral("altrack")).toInt(&ok);
            if (!ok) return { false, QStringLiteral("--altrack requiere un ID de track numerico.") };
            params.asociarAlCrear = true;
            params.trackToAssociate = trackId;
        }

        const TextOperationResult r = service.createLabel(params);
        return { r.ok, r.message };
    }

    return { false, QStringLiteral("Flag no reconocido.\n%1").arg(usage()) };
}
