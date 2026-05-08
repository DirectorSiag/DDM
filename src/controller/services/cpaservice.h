#pragma once

#include <QString>
#include <QMap>

class CommandContext;

struct CPATrackRef {
    bool isOwnShip = false;
    int trackId = -1;
};

struct CPAComputationResult {
    bool valid = false;
    QString errorCode;
    QString errorMessage;
    QString sessionId;
    double tcpaSeconds = 0.0;
    double dcpaDm = 0.0;
    double cpaMidX = 0.0;
    double cpaMidY = 0.0;
};

struct CPASession {
    enum class State {
        Active,
        Finished,
        Expired
    };

    QString sessionId;
    CPATrackRef trackA;
    CPATrackRef trackB;
    State state = State::Active;
};

struct CPAClearResult {
    int removedSessions = 0;
    int removedMarkers = 0;
};

/**
 * @brief Servicio para cálculo y gestión de CPA/PPP entre dos tracks.
 *
 * Centraliza:
 * - resolución de referencias de tracks desde CommandContext
 * - cálculo matemático CPA
 * - administración de sesiones CPA
 * - preparación de marcadores para LPD
 */
class CPAService {
public:
    explicit CPAService(CommandContext* context);

    CPAComputationResult startCPA(const CPATrackRef& trackA, const CPATrackRef& trackB);
    CPAComputationResult graphCPA(const QString& sessionId);
    CPAComputationResult computeCPA(const CPATrackRef& trackA, const CPATrackRef& trackB) const;
    bool finishCPA(const QString& sessionId);
    CPAClearResult clearTrack(const CPATrackRef& trackRef);
    bool isSessionActive(const QString& sessionId) const;

private:
    CommandContext* m_context;
    QMap<QString, CPASession> m_sessions;

    QString buildSessionId(const CPATrackRef& trackA, const CPATrackRef& trackB) const;
    bool resolveTrack(const CPATrackRef& ref,
                      double& xDm,
                      double& yDm,
                      double& speedDmPerHour,
                      double& courseDeg,
                      QString& errorCode) const;
    void upsertMarker(const QString& sessionId,
                      const CPATrackRef& trackA,
                      const CPATrackRef& trackB,
                      double cpaMidX,
                      double cpaMidY) const;
    CPAComputationResult computeFromResolvedStates(const CPATrackRef& trackA,
                                                   const CPATrackRef& trackB,
                                                   double xADm,
                                                   double yADm,
                                                   double speedADmPerHour,
                                                   double courseADeg,
                                                   double xBDm,
                                                   double yBDm,
                                                   double speedBDmPerHour,
                                                   double courseBDeg) const;
};
