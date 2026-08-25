#include "TwoWService.h"
#include "entities/track.h"
#include "model/2w/twoWCalculator.h"
#include "model/2w/twoWStationTable.h"
#include "geometryservice.h"
#include <QStringList>
#include <cmath>

namespace {
// Tipos/colores de trabajo para las figuras de 2W (REQ-2W-LPD-005/006/007).
// El "type" (0-7) es el unico valor que hoy viaja al protocolo binario LPD
// (3 bits de lineType); el mapeo real tipo->color en el renderer queda
// pendiente de coordinar con el equipo de renderer/frontend LPD (ver
// docs/modules/2W-Figuras.md, seccion "Puntos abiertos"). El campo "color"
// solo es visible hoy para un consumidor JSON de list_shapes.
constexpr int kGuideCircleType = 1;
constexpr int kOwnCircleType   = 2;
constexpr int kAllyCircleType  = 3;

const QString kGuideColor = QStringLiteral("#00FF00"); // REQ-2W-LPD-005
const QString kOwnColor   = QStringLiteral("#0000FF"); // REQ-2W-LPD-006
const QString kAllyColor  = QStringLiteral("#FFCC00"); // REQ-2W-LPD-007

// Evita republicar un círculo cuyo centro no cambió desde el último tick
// (ver docs/modules/2W-Figuras.md, "Puntos abiertos" #5): cada llamada a
// GeometryService::updateCircle borra y regenera los 36 CursorEntity del
// círculo, así que saltearla cuando el Guía está estacionario ahorra ese
// churn en cada ciclo de 80 ms mientras la sesión esté activa.
constexpr double kCenterEpsilonDm = 1e-6;

bool centersEqual(const QPointF& a, const QPointF& b) {
    return std::abs(a.x() - b.x()) < kCenterEpsilonDm
        && std::abs(a.y() - b.y()) < kCenterEpsilonDm;
}
}

TwoWService::TwoWService(CommandContext* ctx)
    : m_ctx(ctx)
{}

bool TwoWService::validateStationList(const QList<int>& stations, QString& errorMessage)
{
    for (const int est : stations) {
        if (est < 1 || est > 68) {
            errorMessage = QStringLiteral("Estacion aliada %1 fuera de rango [1, 68].").arg(est);
            return false;
        }
        const TwoWStationEntry& allyData = TwoWStationTable::stationAt(est);
        if (allyData.distanceNm < 0.0) {
            errorMessage = QStringLiteral("La estacion aliada %1 no esta presente en la tabla.").arg(est);
            return false;
        }
    }
    return true;
}

TwoWOperationResult TwoWService::startSession(int guideTrackId, int bpStation, double circleRadiusNm, const QList<int>& aliadas)
{
    if (m_ctx->twoWSession.active) {
        return { false, QStringLiteral("[2W] Ya hay una Disposicion 2W activa. Ejecute 2w --stop primero.") };
    }

    if (bpStation < 1 || bpStation > 68) {
        return { false, QStringLiteral("--est debe estar entre 1 y 68.") };
    }

    const TwoWStationEntry& stationData = TwoWStationTable::stationAt(bpStation);
    if (stationData.distanceNm < 0.0) {
        return { false, QStringLiteral("La estacion %1 no esta presente en la tabla.").arg(bpStation) };
    }

    if (circleRadiusNm <= 0.0) {
        return { false, QStringLiteral("--radio debe ser un numero positivo.") };
    }

    if (!m_ctx->findTrackById(guideTrackId)) {
        return { false, QStringLiteral("No existe un track con ID %1 para usar como Guia.").arg(guideTrackId) };
    }

    // Se valida todo (incluidas las aliadas) antes de mutar cualquier estado,
    // para no dejar la sesion activa a medias si una aliada es invalida.
    QString err;
    if (!validateStationList(aliadas, err)) {
        return { false, err };
    }

    m_ctx->twoWSession.active         = true;
    m_ctx->twoWSession.guideTrackId   = guideTrackId;
    m_ctx->twoWSession.bpStation      = bpStation;
    m_ctx->twoWSession.circleRadiusNm = circleRadiusNm;

    // Calcula posiciones y publica Guia+Propio de inmediato, para que
    // aparezcan en el radar en el mismo instante de INICIAR (REQ-2W-INT-007)
    // y no recien en el siguiente tick de 80 ms. Las aliadas (opcionales,
    // REQ-2W-LPD-002) se publican despues via setStations, que reutiliza
    // este mismo update().
    update();

    if (!aliadas.isEmpty()) {
        setStations(aliadas);
    }

    return { true, QStringLiteral("\n[2W] Disposicion iniciada. Guia=%1 BP-est=%2 radio=%3MN\n")
                      .arg(guideTrackId).arg(bpStation).arg(circleRadiusNm) };
}

TwoWOperationResult TwoWService::setStations(const QList<int>& aliadas)
{
    if (!m_ctx->twoWSession.active) {
        return { false, QStringLiteral("[2W] La Disposicion 2W no se encuentra activa. Ejecute 2w --guia=...--est=... primero.") };
    }

    QString err;
    if (!validateStationList(aliadas, err)) {
        return { false, err };
    }

    m_ctx->twoWSession.selectedStations = aliadas;

    // TwoWCalculator::calculate() + syncFigures() ya hacen el diff
    // create/update/delete de allyCircleIds contra selectedStations.
    update();

    QStringList aliadasStr;
    for (int est : aliadas) aliadasStr << QString::number(est);

    return { true, QStringLiteral("\n[2W] Estaciones aliadas actualizadas: [%1]\n")
                      .arg(aliadasStr.join(QStringLiteral(", "))) };
}

TwoWOperationResult TwoWService::stopSession()
{
    if (!m_ctx->twoWSession.active) {
        return { false, QStringLiteral("[2W] La Disposicion 2W no se encuentra activa en este momento.") };
    }

    deleteAllFigures();
    m_ctx->twoWSession.reset();
    return { true, QStringLiteral("\n[2W] Disposicion finalizada.\n") };
}

void TwoWService::update()
{
    if (!m_ctx->twoWSession.active) return;

    TwoWSessionState& s = m_ctx->twoWSession;
    const Track* guideTrack = m_ctx->findTrackById(s.guideTrackId);
    if (!guideTrack) {
        s.trackValid = false;
        stopSession();
        return;
    }

    s.trackValid = true;
    const QPointF guidePos(guideTrack->getX(), guideTrack->getY());

    QPointF ownPos(0.0, 0.0);
    double  ownSpeed = 0.0;
    const Track* ownTrack = m_ctx->findTrackById(0);
    if (ownTrack) {
        ownPos   = QPointF(ownTrack->getX(), ownTrack->getY());
        ownSpeed = ownTrack->getVelocidadDmPerHour();
    }

    // (a) CALCULO: todos los centros quedan anclados a la posicion ACTUAL
    // del track Guia (guidePos) y a azimut/distancia FIJOS de Tabla A.
    TwoWCalculator::calculate(
        guidePos,
        ownPos, ownSpeed,
        s.bpStation, s.circleRadiusNm, s.selectedStations,
        s.guideCircleCenter, s.ownCircleCenter, s.allyCircleCenters,
        s.etaValid, s.courseToStationDeg, s.timeToStationMin,
        s.currentAzimuthDeg, s.currentDistanceNm,
        s.expectedAzimuthDeg, s.expectedDistanceNm
        );

    // (b) PUBLICACION GRAFICA: solo consume los centros ya calculados en (a).
    syncFigures();
}

void TwoWService::syncFigures()
{
    TwoWSessionState& s = m_ctx->twoWSession;
    GeometryService geometry(m_ctx);
    const double radiusDm = s.circleRadiusNm * TwoWCalculator::kNmToDm;

    // Guía
    if (s.guideCircleId == TwoWSessionState::NO_CIRCLE) {
        const GeometryResult r = geometry.createCircle(s.guideCircleCenter, radiusDm, kGuideCircleType, kGuideColor);
        s.guideCircleId = r.success ? r.id : TwoWSessionState::NO_CIRCLE;
        s.lastSyncedGuideCircleCenter = s.guideCircleCenter;
    } else if (!centersEqual(s.guideCircleCenter, s.lastSyncedGuideCircleCenter)) {
        geometry.updateCircle(s.guideCircleId, s.guideCircleCenter, radiusDm);
        s.lastSyncedGuideCircleCenter = s.guideCircleCenter;
    }

    // Propio (Buque Propio)
    if (s.ownCircleId == TwoWSessionState::NO_CIRCLE) {
        const GeometryResult r = geometry.createCircle(s.ownCircleCenter, radiusDm, kOwnCircleType, kOwnColor);
        s.ownCircleId = r.success ? r.id : TwoWSessionState::NO_CIRCLE;
        s.lastSyncedOwnCircleCenter = s.ownCircleCenter;
    } else if (!centersEqual(s.ownCircleCenter, s.lastSyncedOwnCircleCenter)) {
        geometry.updateCircle(s.ownCircleId, s.ownCircleCenter, radiusDm);
        s.lastSyncedOwnCircleCenter = s.ownCircleCenter;
    }

    // Aliadas: diff entre selectedStations (fuente de verdad) y allyCircleIds.
    while (s.allyCircleIds.size() > s.selectedStations.size()) {
        geometry.deleteCircle(s.allyCircleIds.takeLast());
    }
    for (int i = 0; i < s.selectedStations.size(); ++i) {
        if (i < s.allyCircleIds.size()) {
            const bool unchanged = i < s.lastSyncedAllyCircleCenters.size()
                && centersEqual(s.allyCircleCenters[i], s.lastSyncedAllyCircleCenters[i]);
            if (!unchanged) {
                geometry.updateCircle(s.allyCircleIds[i], s.allyCircleCenters[i], radiusDm);
            }
        } else {
            const GeometryResult r = geometry.createCircle(s.allyCircleCenters[i], radiusDm, kAllyCircleType, kAllyColor);
            s.allyCircleIds.append(r.success ? r.id : TwoWSessionState::NO_CIRCLE);
        }
    }
    s.lastSyncedAllyCircleCenters = s.allyCircleCenters;
}

void TwoWService::deleteAllFigures()
{
    TwoWSessionState& s = m_ctx->twoWSession;
    GeometryService geometry(m_ctx);

    if (s.guideCircleId != TwoWSessionState::NO_CIRCLE) geometry.deleteCircle(s.guideCircleId);
    if (s.ownCircleId   != TwoWSessionState::NO_CIRCLE) geometry.deleteCircle(s.ownCircleId);
    for (int id : s.allyCircleIds) {
        if (id != TwoWSessionState::NO_CIRCLE) geometry.deleteCircle(id);
    }
}