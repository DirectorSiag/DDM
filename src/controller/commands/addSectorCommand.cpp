#include "addSectorCommand.h"
#include "../services/geometryservice.h"
#include "model/entities/sectorEntity.h"

// Convierte string a SectorColor. Devuelve false si no reconoce el valor.
static bool parseSectorColor(const QString& s, SectorColor& out) {
    if (s.compare("RGB1",  Qt::CaseInsensitive) == 0) { out = SectorColor::RGB1;  return true; }
    if (s.compare("RGB2",  Qt::CaseInsensitive) == 0) { out = SectorColor::RGB2;  return true; }
    if (s.compare("RGB3",  Qt::CaseInsensitive) == 0) { out = SectorColor::RGB3;  return true; }
    if (s.compare("CMYK1", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK1; return true; }
    if (s.compare("CMYK2", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK2; return true; }
    if (s.compare("CMYK3", Qt::CaseInsensitive) == 0) { out = SectorColor::CMYK3; return true; }
    return false;
}

CommandResult AddSectorCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    // Uso: addSector <az_izq> <az_der> <rad_int> <rad_ext> <color> <origen_x> <origen_y> [id_track]
    const QStringList& args = inv.args;
    if (args.size() < 7) {
        return {false, "Faltan argumentos. Uso: " + usage()};
    }

    auto toDouble = [](const QString& s, double& out) -> bool {
        bool ok = false;
        out = s.toDouble(&ok);
        return ok;
    };

    double az_izq  = 0.0;
    double az_der  = 0.0;
    double rad_int = 0.0;
    double rad_ext = 0.0;
    double origen_x = 0.0;
    double origen_y = 0.0;

    if (!toDouble(args[0], az_izq))   return {false, "az_izq inválido (número requerido)."};
    if (!toDouble(args[1], az_der))   return {false, "az_der inválido (número requerido)."};
    if (!toDouble(args[2], rad_int))  return {false, "rad_int inválido (número requerido)."};
    if (!toDouble(args[3], rad_ext))  return {false, "rad_ext inválido (número requerido)."};

    SectorColor color;
    if (!parseSectorColor(args[4], color)) {
        return {false, "color inválido. Valores válidos: RGB1, RGB2, RGB3, CMYK1, CMYK2, CMYK3."};
    }

    if (!toDouble(args[5], origen_x)) return {false, "origen_x inválido (número requerido)."};
    if (!toDouble(args[6], origen_y)) return {false, "origen_y inválido (número requerido)."};

    int id_track = 0;
    if (args.size() >= 8) {
        bool ok = false;
        id_track = args[7].toInt(&ok);
        if (!ok) return {false, "id_track inválido (entero requerido)."};
    }

    GeometryService svc(&ctx);
    SectorCreateRequest req;
    req.az_izq   = az_izq;
    req.az_der   = az_der;
    req.rad_int  = rad_int;
    req.rad_ext  = rad_ext;
    req.color    = color;
    req.origen   = QPointF(origen_x, origen_y);
    req.id_track = id_track;

    GeometryResult result = svc.createSector(req);
    if (!result.success) {
        return {false, QString("Error al crear sector: %1").arg(result.message)};
    }

    return {
        true,
        QString("OK addSector → id=%1 az_izq=%2 az_der=%3 rad_int=%4 rad_ext=%5 origen=(%6,%7) id_track=%8")
            .arg(result.id)
            .arg(az_izq,  0, 'f', 2)
            .arg(az_der,  0, 'f', 2)
            .arg(rad_int, 0, 'f', 3)
            .arg(rad_ext, 0, 'f', 3)
            .arg(origen_x, 0, 'f', 3)
            .arg(origen_y, 0, 'f', 3)
            .arg(id_track)
    };
}
