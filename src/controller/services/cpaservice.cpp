#include "cpaservice.h"

#include "commandContext.h"
#include "entities/track.h"
#include "pppcalculator.h"

#include <QtMath>
#include <cmath>

CPAService::CPAService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

QString CPAService::buildSessionId(const CPATrackRef& trackA, const CPATrackRef& trackB) const
{
    const QString left = trackA.isOwnShip ? QStringLiteral("OS") : QString::number(trackA.trackId);
    const QString right = trackB.isOwnShip ? QStringLiteral("OS") : QString::number(trackB.trackId);
    return QStringLiteral("CPA_%1_%2").arg(left, right);
}

bool CPAService::resolveTrack(const CPATrackRef& ref,
                              double& xDm,
                              double& yDm,
                              double& speedDmPerHour,
                              double& courseDeg,
                              QString& errorCode) const
{
    if (ref.isOwnShip) {
        if (!m_context->ownShip.valid) {
            errorCode = QStringLiteral("own_ship_not_set");
            return false;
        }

        xDm = m_context->ownShip.xDm;
        yDm = m_context->ownShip.yDm;
        courseDeg = m_context->ownShip.courseDeg;
        speedDmPerHour = m_context->ownShip.speedKnots / Track::kDmToNm;
        return true;
    }

    const Track* track = m_context->findTrackById(ref.trackId);
    if (!track) {
        errorCode = QStringLiteral("track_not_found");
        return false;
    }

    xDm = track->getX();
    yDm = track->getY();
    speedDmPerHour = track->getVelocidadDmPerHour();
    courseDeg = track->course();
    return true;
}

CPAComputationResult CPAService::computeFromResolvedStates(const CPATrackRef& trackA,
                                                           const CPATrackRef& trackB,
                                                           double xADm,
                                                           double yADm,
                                                           double speedADmPerHour,
                                                           double courseADeg,
                                                           double xBDm,
                                                           double yBDm,
                                                           double speedBDmPerHour,
                                                           double courseBDeg) const
{
    CPAComputationResult out;

    PppCalculator::KinematicState aState;
    aState.xDm = xADm;
    aState.yDm = yADm;
    aState.speedDmPerHour = speedADmPerHour;
    aState.courseDeg = courseADeg;

    PppCalculator::KinematicState bState;
    bState.xDm = xBDm;
    bState.yDm = yBDm;
    bState.speedDmPerHour = speedBDmPerHour;
    bState.courseDeg = courseBDeg;

    const PppCalculator::Result result = PppCalculator::compute(aState, bState);
    if (result.status == PppCalculator::Result::InvalidInput) {
        out.errorCode = QStringLiteral("invalid_kinematic_input");
        out.errorMessage = QStringLiteral("Datos cinemáticos inválidos");
        return out;
    }

    const double tcpaHours = result.timeHours;
    const double aCourseRad = qDegreesToRadians(courseADeg);
    const double bCourseRad = qDegreesToRadians(courseBDeg);

    const double aVx = speedADmPerHour * std::sin(aCourseRad);
    const double aVy = speedADmPerHour * std::cos(aCourseRad);
    const double bVx = speedBDmPerHour * std::sin(bCourseRad);
    const double bVy = speedBDmPerHour * std::cos(bCourseRad);

    if (tcpaHours < 0.0) {
        out.errorCode = QStringLiteral("cpa_expired");
        out.errorMessage = QStringLiteral("El CPA ya ocurrió");
        return out;
    }

    const double aXAtTcpa = xADm + aVx * tcpaHours;
    const double aYAtTcpa = yADm + aVy * tcpaHours;
    const double bXAtTcpa = xBDm + bVx * tcpaHours;
    const double bYAtTcpa = yBDm + bVy * tcpaHours;

    out.valid = true;
    out.tcpaSeconds = tcpaHours * 3600.0;
    out.dcpaDm = result.distanceDm;
    out.cpaMidX = (aXAtTcpa + bXAtTcpa) * 0.5;
    out.cpaMidY = (aYAtTcpa + bYAtTcpa) * 0.5;
    out.errorCode = result.status == PppCalculator::Result::DegenerateRelativeMotion
        ? QStringLiteral("degenerate_relative_motion")
        : QStringLiteral("ok");
    out.errorMessage = result.reason;
    return out;
}

CPAComputationResult CPAService::computeCPA(const CPATrackRef& trackA, const CPATrackRef& trackB) const
{
    CPAComputationResult out;

    if (!trackA.isOwnShip && !trackB.isOwnShip && trackA.trackId == trackB.trackId) {
        out.errorCode = QStringLiteral("same_track");
        out.errorMessage = QStringLiteral("CPA requiere dos tracks distintos");
        return out;
    }

    if (trackB.isOwnShip) {
        out.errorCode = QStringLiteral("own_ship_not_allowed_in_track_b");
        out.errorMessage = QStringLiteral("OwnShip solo puede participar como track_a");
        return out;
    }

    double xA = 0.0;
    double yA = 0.0;
    double speedA = 0.0;
    double courseA = 0.0;
    QString errorCodeA;
    if (!resolveTrack(trackA, xA, yA, speedA, courseA, errorCodeA)) {
        out.errorCode = errorCodeA;
        out.errorMessage = (errorCodeA == QStringLiteral("own_ship_not_set"))
            ? QStringLiteral("OwnShip no está configurado")
            : QStringLiteral("Track A no encontrado");
        return out;
    }

    double xB = 0.0;
    double yB = 0.0;
    double speedB = 0.0;
    double courseB = 0.0;
    QString errorCodeB;
    if (!resolveTrack(trackB, xB, yB, speedB, courseB, errorCodeB)) {
        out.errorCode = errorCodeB;
        out.errorMessage = QStringLiteral("Track B no encontrado");
        return out;
    }

    return computeFromResolvedStates(trackA, trackB, xA, yA, speedA, courseA, xB, yB, speedB, courseB);
}

void CPAService::upsertMarker(const QString& sessionId,
                              const CPATrackRef& trackA,
                              const CPATrackRef& trackB,
                              double cpaMidX,
                              double cpaMidY) const
{
    CommandContext::CpaMarkerState marker;
    marker.sessionId = sessionId;
    marker.trackAId = trackA.isOwnShip ? -1 : trackA.trackId;
    marker.trackBId = trackB.isOwnShip ? -1 : trackB.trackId;
    marker.xDm = static_cast<float>(cpaMidX);
    marker.yDm = static_cast<float>(cpaMidY);
    marker.visible = true;
    m_context->upsertCpaMarker(marker);
}

CPAComputationResult CPAService::startCPA(const CPATrackRef& trackA, const CPATrackRef& trackB)
{
    CPAComputationResult result = computeCPA(trackA, trackB);
    if (!result.valid) {
        return result;
    }

    result.sessionId = buildSessionId(trackA, trackB);

    CPASession session;
    session.sessionId = result.sessionId;
    session.trackA = trackA;
    session.trackB = trackB;
    session.state = CPASession::State::Active;
    m_sessions[result.sessionId] = session;
    return result;
}

CPAComputationResult CPAService::graphCPA(const QString& sessionId)
{
    CPAComputationResult out;
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        out.errorCode = QStringLiteral("session_not_found");
        out.errorMessage = QStringLiteral("No existe la sesión CPA");
        return out;
    }

    const CPASession& session = it.value();
    if (session.state != CPASession::State::Active) {
        out.errorCode = QStringLiteral("session_inactive");
        out.errorMessage = (session.state == CPASession::State::Expired)
            ? QStringLiteral("La sesión CPA ya expiró")
            : QStringLiteral("La sesión CPA está inactiva");
        return out;
    }

    out = computeCPA(session.trackA, session.trackB);
    if (out.valid) {
        out.sessionId = sessionId;
        upsertMarker(sessionId, session.trackA, session.trackB, out.cpaMidX, out.cpaMidY);
    } else if (out.errorCode == QStringLiteral("cpa_expired")) {
        it->state = CPASession::State::Expired;
        m_context->eraseCpaMarkerBySessionId(sessionId);
        out.sessionId = sessionId;
    }
    return out;
}

bool CPAService::finishCPA(const QString& sessionId)
{
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }

    it->state = CPASession::State::Finished;
    m_context->eraseCpaMarkerBySessionId(sessionId);
    return true;
}

CPAClearResult CPAService::clearTrack(const CPATrackRef& trackRef)
{
    CPAClearResult result;

    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        const bool matchesTrackA = trackRef.isOwnShip
            ? it->trackA.isOwnShip
            : (!it->trackA.isOwnShip && it->trackA.trackId == trackRef.trackId);
        const bool matchesTrackB = !trackRef.isOwnShip
            && !it->trackB.isOwnShip
            && it->trackB.trackId == trackRef.trackId;

        if (matchesTrackA || matchesTrackB) {
            m_context->eraseCpaMarkerBySessionId(it.key());
            it = m_sessions.erase(it);
            ++result.removedSessions;
        } else {
            ++it;
        }
    }

    result.removedMarkers = trackRef.isOwnShip
        ? m_context->eraseCpaMarkersByTrackId(-1)
        : m_context->eraseCpaMarkersByTrackId(trackRef.trackId);

    return result;
}

bool CPAService::isSessionActive(const QString& sessionId) const
{
    auto it = m_sessions.find(sessionId);
    return it != m_sessions.end() && it.value().state == CPASession::State::Active;
}
