#include "haService.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>
#include "model/ha/haCalculator.h"

HaService::HaService(CommandContext* ctx)
    : m_ctx(ctx)
{}

// ── Disparadores ──────────────────────────────────────────────────────────

HaOperationResult HaService::startSessionAtOwnShip() {
    const Track* bp = m_ctx->findTrackById(0);
    if (!bp) {
        return { false, QStringLiteral("[HA] Error: no se encontro el Buque Propio.") };
    }

    initSession(bp->getX(), bp->getY());
    return { true, QStringLiteral("[HA] Emergencia iniciada a popa del Buque Propio.") };
}

HaOperationResult HaService::startSessionAtCursor(double cursorXDm, double cursorYDm) {
    initSession(cursorXDm, cursorYDm);
    return { true, QStringLiteral("[HA] Emergencia iniciada sobre el cursor.") };
}

HaOperationResult HaService::startSessionAtLatLonDms(
    int latDeg, int latMin, double latSec,
    int lonDeg, int lonMin, double lonSec)
{
    const double lat = RadarMath::dmsToDecimal(latDeg, latMin, latSec);
    const double lon = RadarMath::dmsToDecimal(lonDeg, lonMin, lonSec);

    return startSessionAtLatLon(lat, lon);  // reusa la validación y lógica existente
}

// Este queda privado, sin cambios en su cuerpo:
HaOperationResult HaService::startSessionAtLatLon(double lat, double lon) {
    if (lat < -90.0 || lat > 90.0) {
        return { false, QStringLiteral("--lat debe estar en el rango [-90, 90].") };
    }
    if (lon < -180.0 || lon > 180.0) {
        return { false, QStringLiteral("--lon debe estar en el rango [-180, 180].") };
    }

    const bool bpHasGeo = m_ctx->ownShip.valid &&
                          !(m_ctx->ownShip.latitudeDeg == 0.0 &&
                            m_ctx->ownShip.longitudeDeg == 0.0);

    if (!bpHasGeo) {
        return { false, QStringLiteral(
                           "[HA] Disparador Lat/Lon no disponible: el BP no tiene coordenadas geograficas reales.") };
    }

    double xDm = 0.0, yDm = 0.0;
    RadarMath::latLonToDm(
        m_ctx->ownShip.latitudeDeg,
        m_ctx->ownShip.longitudeDeg,
        lat, lon,
        xDm, yDm
        );

    initSession(xDm, yDm);
    return { true, QStringLiteral("[HA] Emergencia iniciada por Lat/Lon.") };
}
HaOperationResult HaService::startSessionAtBearing(double azimuthDeg, double distanceYards) {
    if (azimuthDeg < 0.0 || azimuthDeg >= 360.0) {
        return { false, QStringLiteral("--az debe estar en el rango [0, 360).") };
    }
    if (distanceYards <= 0.0) {
        return { false, QStringLiteral("--d debe ser un numero positivo (en yardas).") };
    }

    const Track* bp = m_ctx->findTrackById(0);
    if (!bp) {
        return { false, QStringLiteral("[HA] Error: no se encontro el Buque Propio.") };
    }

    const double distDm = RadarMath::yardsToDm(distanceYards);
    const double azRad  = qDegreesToRadians(azimuthDeg);
    const double xDm    = bp->getX() + distDm * std::sin(azRad);
    const double yDm    = bp->getY() + distDm * std::cos(azRad);

    initSession(xDm, yDm);
    return { true, QStringLiteral("[HA] Emergencia iniciada por azimut y distancia.") };
}

// ── Sesión ────────────────────────────────────────────────────────────────

void HaService::initSession(double xDm, double yDm) {
    m_ctx->haSession.reset();

    m_ctx->haSession.active     = true;
    m_ctx->haSession.fallPointX = xDm;
    m_ctx->haSession.fallPointY = yDm;

    m_timer.start();

    m_ctx->haSession.fallTimeLocal = m_timer.fallTimeLocal();
    m_ctx->haSession.fallTimeUtc   = m_timer.fallTimeUtc();

    m_ctx->out << QStringLiteral("[HA] Punto fijado en (%1, %2) DM.\n")
                      .arg(xDm, 0, 'f', 2)
                      .arg(yDm, 0, 'f', 2);
    m_ctx->out.flush();
}

HaOperationResult HaService::stopSession() {
    if (!m_ctx->haSession.active) {
        return { false, QStringLiteral("[HA] No hay ninguna emergencia activa en este momento.") };
    }

    m_ctx->haSession.reset();
    m_timer.reset();

    return { true, QStringLiteral("[HA] Emergencia finalizada.") };
}

void HaService::update() {
    if (!m_ctx->haSession.active) return;

    const Track* bp = m_ctx->findTrackById(0);
    double bpX = 0.0, bpY = 0.0, bpCourse = 0.0, bpSpeed = 0.0;

    if (bp) {
        bpX      = bp->getX();
        bpY      = bp->getY();
        bpCourse = bp->getCourseDeg();
        bpSpeed  = bp->getVelocidadDmPerHour();
    }

    const QPointF ownPos(bpX, bpY);
    const QPointF fallPoint(m_ctx->haSession.fallPointX,
                            m_ctx->haSession.fallPointY);

    HaCalculator::calculate(ownPos, bpCourse, bpSpeed, fallPoint, m_ctx->haSession);

    m_ctx->haSession.elapsedTime = m_timer.elapsedTime();
}