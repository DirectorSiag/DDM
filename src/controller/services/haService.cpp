#include "haService.h"
#include "model/utils/RadarMath.h"
#include <QtMath>
#include <cmath>
#include "model/ha/haCalculator.h"
#include "obmservice.h"

HaService::HaService(CommandContext* ctx, ObmService* obmService)
    : m_ctx(ctx), m_obmService(obmService)
{}

// ── Helper ────────────────────────────────────────────────────────────────

int HaService::nextFreeSlot() const {
    for (int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
        if (!m_ctx->haSessions[i].active) return i + 1;  // 1-based
    }
    return -1;  // todos ocupados
}

// ── Disparadores ──────────────────────────────────────────────────────────

HaOperationResult HaService::startSessionAtOwnShip() {
    const Track* bp = m_ctx->findTrackById(0);
    if (!bp) {
        return { false, QStringLiteral("[HA] Error: no se encontro el Buque Propio.") };
    }
    initSession(bp->getX(), bp->getY());
    return { true, QString() };
}

HaOperationResult HaService::startSessionAtCursor() {
    const QPair<double, double> obmPos = m_obmService->getCurrentPosition();
    initSession(obmPos.first, obmPos.second);
    return { true, QString() };
}

HaOperationResult HaService::startSessionAtLatLonDms(
    int latDeg, int latMin, double latSec,
    int lonDeg, int lonMin, double lonSec)
{
    const double lat = RadarMath::dmsToDecimal(latDeg, latMin, latSec);
    const double lon = RadarMath::dmsToDecimal(lonDeg, lonMin, lonSec);
    return startSessionAtLatLon(lat, lon);
}

HaOperationResult HaService::startSessionAtLatLon(double lat, double lon) {
    if (lat < -90.0 || lat > 90.0)
        return { false, QStringLiteral("--lat debe estar en el rango [-90, 90].") };
    if (lon < -180.0 || lon > 180.0)
        return { false, QStringLiteral("--lon debe estar en el rango [-180, 180].") };

    const bool bpHasGeo = m_ctx->ownShip.valid &&
                          !(m_ctx->ownShip.latitudeDeg == 0.0 &&
                            m_ctx->ownShip.longitudeDeg == 0.0);
    if (!bpHasGeo) {
        return { false, QStringLiteral(
            "[HA] Disparador Lat/Lon no disponible: el BP no tiene coordenadas geograficas reales.") };
    }

    double xDm = 0.0, yDm = 0.0;
    RadarMath::latLonToDm(
        m_ctx->ownShip.latitudeDeg, m_ctx->ownShip.longitudeDeg,
        lat, lon, xDm, yDm
    );

    initSession(xDm, yDm);
    return { true, QString() };
}

HaOperationResult HaService::startSessionAtBearing(double azimuthDeg, double distanceYards) {
    if (azimuthDeg < 0.0 || azimuthDeg >= 360.0)
        return { false, QStringLiteral("--az debe estar en el rango [0, 360).") };
    if (distanceYards <= 0.0)
        return { false, QStringLiteral("--d debe ser un numero positivo (en yardas).") };

    const Track* bp = m_ctx->findTrackById(0);
    if (!bp)
        return { false, QStringLiteral("[HA] Error: no se encontro el Buque Propio.") };

    const double distDm = RadarMath::yardsToDm(distanceYards);
    const double azRad  = qDegreesToRadians(azimuthDeg);
    const double xDm    = bp->getX() + distDm * std::sin(azRad);
    const double yDm    = bp->getY() + distDm * std::cos(azRad);

    initSession(xDm, yDm);
    return { true, QString() };
}

// ── Sesión ────────────────────────────────────────────────────────────────

void HaService::initSession(double xDm, double yDm) {
    const int slot = nextFreeSlot();
    if (slot == -1) {
        m_ctx->out << "[HA] No hay slots disponibles (maximo 10 emergencias activas).\n";
        m_ctx->out.flush();
        return;
    }

    const int idx = slot - 1;  // 0-based para el array

    m_ctx->haSessions[idx].reset();
    m_ctx->haSessions[idx].active     = true;
    m_ctx->haSessions[idx].slotIndex  = slot;
    m_ctx->haSessions[idx].fallPointX = xDm;
    m_ctx->haSessions[idx].fallPointY = yDm;

    m_ctx->haSessions[idx].timer.start();
    m_ctx->haSessions[idx].fallTimeLocal = m_ctx->haSessions[idx].timer.fallTimeLocal();
    m_ctx->haSessions[idx].fallTimeUtc   = m_ctx->haSessions[idx].timer.fallTimeUtc();

    // Seleccionar automáticamente el slot recién creado
    m_ctx->activeHaSlot = slot;

    m_ctx->out << QStringLiteral("[HA] Emergencia iniciada en slot %1. Punto fijado en (%2, %3) DM.\n")
                      .arg(slot)
                      .arg(xDm, 0, 'f', 2)
                      .arg(yDm, 0, 'f', 2);
    m_ctx->out.flush();
}

HaOperationResult HaService::stopSession(int slotIndex) {
    // --stop sin argumento: finalizar todos
    if (slotIndex == -1) {
        bool anyActive = false;
        for (int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
            if (m_ctx->haSessions[i].active) {
                m_ctx->haSessions[i].reset();
                anyActive = true;
            }
        }
        m_ctx->activeHaSlot = -1;
        if (!anyActive)
            return { false, QStringLiteral("[HA] No hay ninguna emergencia activa.") };
        return { true, QStringLiteral("[HA] Todas las emergencias finalizadas.") };
    }

    // --stop <slot>: finalizar slot específico
    if (slotIndex < 1 || slotIndex > CommandContext::kMaxHaSessions)
        return { false, QStringLiteral("[HA] Slot invalido. Debe estar entre 1 y 10.") };

    const int idx = slotIndex - 1;
    if (!m_ctx->haSessions[idx].active)
        return { false, QStringLiteral("[HA] El slot %1 no tiene ninguna emergencia activa.").arg(slotIndex) };

    m_ctx->haSessions[idx].reset();

    if (m_ctx->activeHaSlot == slotIndex)
        m_ctx->activeHaSlot = -1;

    return { true, QStringLiteral("[HA] Emergencia del slot %1 finalizada.").arg(slotIndex) };
}

HaOperationResult HaService::infoReport(int slotIndex) const {
    if (slotIndex < 1 || slotIndex > CommandContext::kMaxHaSessions)
        return { false, QStringLiteral("[HA] Slot invalido. Debe estar entre 1 y 10.") };

    const int idx = slotIndex - 1;
    if (!m_ctx->haSessions[idx].active)
        return { false, QStringLiteral("[HA] El slot %1 no tiene ninguna emergencia activa.").arg(slotIndex) };

    const HaSessionState& s = m_ctx->haSessions[idx];
    QString response;
    response += QStringLiteral("\n======================================================\n");
    response += QStringLiteral("          HOMBRE AL AGUA — SLOT %1\n").arg(slotIndex);
    response += QStringLiteral("======================================================\n");
    response += QStringLiteral("Hora de Caida (Local): %1  |  UTC: %2\n")
                    .arg(s.fallTimeLocal).arg(s.fallTimeUtc);
    response += QStringLiteral("Tiempo Transcurrido:   %1\n").arg(s.elapsedTime);
    response += QStringLiteral("------------------------------------------------------\n");
    response += QStringLiteral("Azimut Verdadero:      %1 grados\n")
                    .arg(s.trueAzimuthDeg, 0, 'f', 1);
    response += QStringLiteral("Marcacion Relativa:    %1 grados  (%2)\n")
                    .arg(s.relativeBearingDeg, 0, 'f', 1).arg(s.banda);
    response += QStringLiteral("Distancia:             %1 yardas\n")
                    .arg(s.distanceYards, 0, 'f', 0);
    if (s.etaValid) {
        response += QStringLiteral("\n--> TIEMPO DE ARRIBO: %1 minutos\n")
                        .arg(s.timeToArrivalMin, 0, 'f', 1);
    } else {
        response += QStringLiteral("\n--> TIEMPO DE ARRIBO: N/D (velocidad en 0)\n");
    }
    response += QStringLiteral("======================================================\n\n");

    return { true, response };
}

HaOperationResult HaService::selectSlot(int slotIndex) {
    if (slotIndex < 1 || slotIndex > CommandContext::kMaxHaSessions)
        return { false, QStringLiteral("[HA] Slot invalido. Debe estar entre 1 y 10.") };

    const int idx = slotIndex - 1;
    if (!m_ctx->haSessions[idx].active)
        return { false, QStringLiteral("[HA] El slot %1 no tiene ninguna emergencia activa.").arg(slotIndex) };

    m_ctx->activeHaSlot = slotIndex;
    return { true, QStringLiteral("[HA] Slot %1 seleccionado.").arg(slotIndex) };
}

void HaService::update() {
    const Track* bp = m_ctx->findTrackById(0);
    double bpX = 0.0, bpY = 0.0, bpCourse = 0.0, bpSpeed = 0.0;

    if (bp) {
        bpX      = bp->getX();
        bpY      = bp->getY();
        bpCourse = bp->getCourseDeg();
    }

    if (m_ctx->ownShip.valid && m_ctx->ownShip.speedKnots > 0.0)
        bpSpeed = m_ctx->ownShip.speedKnots / Track::kDmToNm;

    const QPointF ownPos(bpX, bpY);

    // Recalcular todos los slots activos
    for (int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
        if (!m_ctx->haSessions[i].active) continue;

        const QPointF fallPoint(m_ctx->haSessions[i].fallPointX,
                                m_ctx->haSessions[i].fallPointY);

        HaCalculator::calculate(ownPos, bpCourse, bpSpeed, fallPoint, m_ctx->haSessions[i]);

        m_ctx->haSessions[i].elapsedTime = m_ctx->haSessions[i].timer.elapsedTime();
    }
}
