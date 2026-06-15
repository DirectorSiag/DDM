#include "haService.h"
#include "model/utils/RadarMath.h"
#include "model/ha/haCalculator.h"
#include <QtMath>
#include <cmath>

HaService::HaService(CommandContext* ctx)
    : m_ctx(ctx)
{}

// Disparadores

void HaService::startSessionAtOwnShip() {
    // El punto de caída es la posición actual del BP (Track 0)
    const Track* bp = m_ctx->findTrackById(0);
    if (!bp) {
        m_ctx->out << "[HA] Error: no se encontro el Buque Propio.\n";
        m_ctx->out.flush();
        return;
    }
    initSession(bp->getX(), bp->getY());
}

void HaService::startSessionAtCursor(double cursorXDm, double cursorYDm) {
    initSession(cursorXDm, cursorYDm);
}

void HaService::startSessionAtLatLon(double lat, double lon) {
    // TODO: convertir lat/lon a coordenadas DM del radar
    // Por ahora se almacena directamente como placeholder
    Q_UNUSED(lat)
    Q_UNUSED(lon)
    m_ctx->out << "[HA] Disparador Lat/Lon pendiente de implementacion de conversion.\n";
    m_ctx->out.flush();
}

void HaService::startSessionAtBearing(double azimuthDeg, double distanceYards) {
    // Proyectar el punto desde la posición actual del BP
    const Track* bp = m_ctx->findTrackById(0);
    if (!bp) {
        m_ctx->out << "[HA] Error: no se encontro el Buque Propio.\n";
        m_ctx->out.flush();
        return;
    }

    const double distDm  = RadarMath::yardsToDm(distanceYards);
    const double azRad   = qDegreesToRadians(azimuthDeg);
    const double xDm     = bp->getX() + distDm * std::sin(azRad);
    const double yDm     = bp->getY() + distDm * std::cos(azRad);

    initSession(xDm, yDm);
}

// Sesión

void HaService::initSession(double xDm, double yDm) {
    // Sobreescribe cualquier sesión anterior al comenzar
    m_ctx->haSession.reset();

    m_ctx->haSession.active     = true;
    m_ctx->haSession.fallPointX = xDm;
    m_ctx->haSession.fallPointY = yDm;

    m_timer.start();

    // Escribir hora de caída congelada
    m_ctx->haSession.fallTimeLocal = m_timer.fallTimeLocal();
    m_ctx->haSession.fallTimeUtc   = m_timer.fallTimeUtc();

    m_ctx->out << QStringLiteral("[HA] Emergencia iniciada. Punto fijado en (%1, %2) DM.\n")
                      .arg(xDm, 0, 'f', 2)
                      .arg(yDm, 0, 'f', 2);
    m_ctx->out.flush();
}

void HaService::stopSession() {
    m_ctx->haSession.reset();
    m_timer.reset();

    m_ctx->out << "[HA] Emergencia finalizada.\n";
    m_ctx->out.flush();
}

void HaService::update() {
    if (!m_ctx->haSession.active) return;

    // Obtener posición y cinemática del BP
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

    // Calcular asesoramiento
    HaCalculator::calculate(ownPos, bpCourse, bpSpeed, fallPoint, m_ctx->haSession);

    // Actualizar cronómetro
    m_ctx->haSession.elapsedTime = m_timer.elapsedTime();
}