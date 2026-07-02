#include "sectorEntity.h"
#include "model/commandContext.h"
#include "model/entities/cursorEntity.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>

SectorEntity::SectorEntity(int id,
                           double az_izq,
                           double az_der,
                           double rad_int,
                           double rad_ext,
                           SectorTipo tipo,
                           SectorColor color,
                           const QPointF& origen,
                           int id_track)
    : id(id)
    , az_izq(az_izq)
    , az_der(az_der)
    , rad_int(rad_int)
    , rad_ext(rad_ext)
    , tipo(tipo)
    , color(color)
    , origen(origen)
    , id_track(id_track)
{}

bool SectorEntity::isValid() const {
    if (rad_ext <= 0.0)           return false;
    if (rad_int < 0.0)            return false;
    if (rad_ext <= rad_int)       return false;
    if (az_izq < 0.0 || az_izq >= 360.0) return false;
    if (az_der < 0.0 || az_der >= 360.0) return false;
    return true;
}

double SectorEntity::apertura() const {
    // AZ_IZQ == AZ_DER → corona completa (360°)
    if (az_izq == az_der) return 360.0;

    double ap = az_der - az_izq;
    if (ap <= 0.0) ap += 360.0;
    return ap;
}

void SectorEntity::calculateAndStoreCursors(CommandContext& ctx) {
    cursorIds.clear();

    const int lineType = static_cast<int>(color);

    auto addSegment = [&](const QPointF& p1, const QPointF& p2) {
        qfloat16 angle  = RadarMath::calculateAngle(p1, p2);
        qfloat16 length = RadarMath::calculateLength(p1, p2);
        CursorEntity c(
            QPair<qfloat16, qfloat16>(static_cast<qfloat16>(p1.x()), static_cast<qfloat16>(p1.y())),
            angle, length, lineType, ctx.nextCursorId++, true
        );
        ctx.addCursorFront(c);
        cursorIds.push_back(c.getCursorId());
    };

    // Azimut náutico (N=0°, horario) → coordenadas cartesianas
    auto azToPoint = [&](double az_deg, double r) -> QPointF {
        const double rad = qDegreesToRadians(az_deg);
        return QPointF(origen.x() + r * std::sin(rad),
                       origen.y() + r * std::cos(rad));
    };

    const double ap = apertura();
    const int n = std::max(1, static_cast<int>(ap / 10.0));
    const double step = ap / n;

    // Arco exterior: az_izq → az_der en sentido horario, a rad_ext
    for (int i = 0; i < n; ++i) {
        double a1 = std::fmod(az_izq + i       * step, 360.0);
        double a2 = std::fmod(az_izq + (i + 1) * step, 360.0);
        addSegment(azToPoint(a1, rad_ext), azToPoint(a2, rad_ext));
    }

    // Arco interior: az_izq → az_der, a rad_int (solo si hay anillo)
    if (rad_int > 0.0) {
        for (int i = 0; i < n; ++i) {
            double a1 = std::fmod(az_izq + i       * step, 360.0);
            double a2 = std::fmod(az_izq + (i + 1) * step, 360.0);
            addSegment(azToPoint(a1, rad_int), azToPoint(a2, rad_int));
        }
    }

    // Líneas radiales de cierre (solo si no es corona completa de 360°)
    if (ap < 360.0) {
        QPointF inner_izq = (rad_int > 0.0) ? azToPoint(az_izq, rad_int) : origen;
        QPointF inner_der = (rad_int > 0.0) ? azToPoint(az_der, rad_int) : origen;
        addSegment(inner_izq, azToPoint(az_izq, rad_ext));
        addSegment(inner_der, azToPoint(az_der, rad_ext));
    }
}

int SectorEntity::getId() const        { return id; }
double SectorEntity::getAzIzq() const  { return az_izq; }
double SectorEntity::getAzDer() const  { return az_der; }
double SectorEntity::getRadInt() const { return rad_int; }
double SectorEntity::getRadExt() const { return rad_ext; }
SectorTipo  SectorEntity::getTipo()  const { return tipo; }
SectorColor SectorEntity::getColor() const { return color; }
const QPointF& SectorEntity::getOrigen() const { return origen; }
int SectorEntity::getIdTrack() const   { return id_track; }

void SectorEntity::setAzIzq(double az)          { az_izq = az; }
void SectorEntity::setAzDer(double az)          { az_der = az; }
void SectorEntity::setRadInt(double rad)        { rad_int = rad; }
void SectorEntity::setRadExt(double rad)        { rad_ext = rad; }
void SectorEntity::setTipo(SectorTipo t)        { tipo = t; }
void SectorEntity::setColor(SectorColor c)      { color = c; }
void SectorEntity::setOrigen(const QPointF& o)  { origen = o; }
void SectorEntity::setIdTrack(int id)           { id_track = id; }

const std::vector<int>& SectorEntity::getCursorIds() const { return cursorIds; }
