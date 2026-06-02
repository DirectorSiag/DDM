#include "twowservice.h"
#include "entities/track.h"
#include "model/2w/twoWCalculator.h"

TwoWService::TwoWService(CommandContext* ctx)
    : m_ctx(ctx)
{}

void TwoWService::startSession(int guideTrackId, int bpStation, double circleRadiusNm)
{
    m_ctx->twoWSession.active         = true;
    m_ctx->twoWSession.guideTrackId   = guideTrackId;
    m_ctx->twoWSession.bpStation      = bpStation;
    m_ctx->twoWSession.circleRadiusNm = circleRadiusNm;

    m_ctx->out << QStringLiteral("\n[2W] Disposicion iniciada. Guia=%1 BP-est=%2 radio=%3MN\n")
                      .arg(guideTrackId).arg(bpStation).arg(circleRadiusNm);
    m_ctx->out.flush();
}

void TwoWService::stopSession()
{
    m_ctx->twoWSession.reset();

    m_ctx->out << QStringLiteral("\n[2W] Disposicion finalizada.\n");
    m_ctx->out.flush();
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

    // Buque propio: track 0
    QPointF ownPos(0.0, 0.0);
    double  ownSpeed = 0.0;
    const Track* ownTrack = m_ctx->findTrackById(0);
    if (ownTrack) {
        ownPos   = QPointF(ownTrack->getX(), ownTrack->getY());
        ownSpeed = ownTrack->getVelocidadDmPerHour();
    }

    // Hacer refactor
    TwoWCalculator::calculate(
        guidePos, guideTrack->getCourseDeg(), guideTrack->getVelocidadDmPerHour(),
        ownPos,   ownSpeed,
        s.bpStation, s.circleRadiusNm, s.selectedStations,
        s.guideCircleCenter, s.ownCircleCenter, s.allyCircleCenters,
        s.etaValid,   s.courseToStationDeg, s.timeToStationMin,
        s.currentAzimuthDeg, s.currentDistanceNm,
        s.expectedAzimuthDeg, s.expectedDistanceNm
        );

}

