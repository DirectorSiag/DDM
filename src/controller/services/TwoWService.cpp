#include "twowservice.h"
#include "entities/track.h"
#include "model/2w/twoWCalculator.h"
#include "model/2w/twoWStationTable.h"

TwoWService::TwoWService(CommandContext* ctx)
    : m_ctx(ctx)
{}

TwoWOperationResult TwoWService::startSession(int guideTrackId, int bpStation, double circleRadiusNm, const QList<int>& aliadas)
{
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

    for (const int est : aliadas) {
        if (est < 1 || est > 68) {
            return { false, QStringLiteral("Estacion aliada %1 fuera de rango [1, 68].").arg(est) };
        }
        const TwoWStationEntry& allyData = TwoWStationTable::stationAt(est);
        if (allyData.distanceNm < 0.0) {
            return { false, QStringLiteral("La estacion aliada %1 no esta presente en la tabla.").arg(est) };
        }
    }

    m_ctx->twoWSession.active         = true;
    m_ctx->twoWSession.guideTrackId   = guideTrackId;
    m_ctx->twoWSession.bpStation      = bpStation;
    m_ctx->twoWSession.circleRadiusNm = circleRadiusNm;
    m_ctx->twoWSession.selectedStations = aliadas;

    return { true, QStringLiteral("\n[2W] Disposicion iniciada. Guia=%1 BP-est=%2 radio=%3MN\n")
                      .arg(guideTrackId).arg(bpStation).arg(circleRadiusNm) };
}

TwoWOperationResult TwoWService::stopSession()
{
    if (!m_ctx->twoWSession.active) {
        return { false, QStringLiteral("[2W] La Disposicion 2W no se encuentra activa en este momento.") };
    }

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

    TwoWCalculator::calculate(
        guidePos,
        ownPos, ownSpeed,
        s.bpStation, s.circleRadiusNm, s.selectedStations,
        s.guideCircleCenter, s.ownCircleCenter, s.allyCircleCenters,
        s.etaValid, s.courseToStationDeg, s.timeToStationMin,
        s.currentAzimuthDeg, s.currentDistanceNm,
        s.expectedAzimuthDeg, s.expectedDistanceNm
        );
}