#include "derrotasService.h"
#include "model/derrotas/futuraCalculator.h"
#include "RadarMath.h"
#include <QDateTime>

namespace {
// PENDIENTE de confirmar: tomamos un intervalo de segmentacion de 15 min
constexpr int kDefaultSegmentMinutes = 15;

// PENDIENTE de confirmar: ruta real en el SSD
const QString kLogDirectory = QStringLiteral("./logs/derrotas");
}

DerrotasService::DerrotasService(CommandContext* ctx)
    : m_ctx(ctx), m_logManager(kLogDirectory)
{}

bool DerrotasService::isValidTime(int minutes) const {
    return minutes == 5 || minutes == 10 || minutes == 30 || minutes == 60;
}

bool DerrotasService::isValidThresholdDeg(double deg) const {
    return deg == 10.0 || deg == 30.0 || deg == 45.0 || deg == 90.0 || deg == 150.0 || deg == 180.0;
}

bool DerrotasService::isValidThresholdKn(double kn) const {
    return kn == 2.0 || kn == 5.0 || kn == 10.0 || kn == 20.0 || kn == 50.0 || kn == 100.0;
}

// ── Derrotas Futuras ─────────────────────────────────────────────────────

DerrotasOperationResult DerrotasService::startFutura(int trackId, int timeMinutes, double thresholdDeg, double thresholdKn)
{
    if (m_ctx->derrotasSession.futura.active) {
        return { false, QStringLiteral("[Derrotas] Ya hay un asesoramiento futuro activo. Ejecute FINALIZAR primero.") };
    }

    const Track* track = m_ctx->findTrackById(trackId);
    if (!track) {
        return { false, QStringLiteral("[Derrotas] Error: el track %1 no existe.").arg(trackId) };
    }

    if (!isValidTime(timeMinutes)) {
        return { false, QStringLiteral("[Derrotas] TIEMPO invalido. Valores permitidos: 5, 10, 30, 60 minutos.") };
    }
    if (!isValidThresholdDeg(thresholdDeg)) {
        return { false, QStringLiteral("[Derrotas] Umbral de GRADOS invalido.") };
    }
    if (!isValidThresholdKn(thresholdKn)) {
        return { false, QStringLiteral("[Derrotas] Umbral de NUDOS invalido.") };
    }

    const double courseDeg = track->getCourseDeg();
    const double speedKn   = track->getVelocidadDmPerHour() * Track::kDmToNm;

    QList<QPointF> crossPoints;
    double totalDistanceDm = 0.0;
    FuturaCalculator::calculateProjection(
        QPointF(track->getX(), track->getY()), courseDeg, speedKn, timeMinutes,
        crossPoints, totalDistanceDm);

    DerrotasFuturaState& f = m_ctx->derrotasSession.futura;
    f.reset();
    f.active = true;
    f.config = { trackId, timeMinutes, thresholdDeg, thresholdKn };

    //foto cinematica tomada al INICIAR — ancla de la alarma.
    f.baseCourseDeg = courseDeg;
    f.baseSpeedKn   = speedKn;

    f.crossPoints      = crossPoints;
    f.totalDistanceDm  = totalDistanceDm;

    return { true, QStringLiteral("[Derrotas] Asesoramiento futuro iniciado para el track %1.").arg(trackId) };
}

DerrotasOperationResult DerrotasService::stopFutura()
{
    if (!m_ctx->derrotasSession.futura.active) {
        return { false, QStringLiteral("[Derrotas] No hay ningun asesoramiento futuro activo.") };
    }
    m_ctx->derrotasSession.futura.reset();
    return { true, QStringLiteral("[Derrotas] Asesoramiento futuro finalizado.") };
}

// ── Derrotas Pasadas ─────────────────────────────────────────────────────

DerrotasOperationResult DerrotasService::startRecording(int trackId)
{
    DerrotasPasadaState& p = m_ctx->derrotasSession.pasada;

    if (p.recordingActive) {
        return { false, QStringLiteral("[Derrotas] Ya hay una grabacion en curso. Ejecute FINALIZAR primero.") };
    }

    const Track* track = m_ctx->findTrackById(trackId);
    if (!track) {
        return { false, QStringLiteral("[Derrotas] Error: el track %1 no existe.").arg(trackId) };
    }

    const QDateTime now = QDateTime::currentDateTime();

    if (!m_logManager.startLog(trackId, now, kDefaultSegmentMinutes)) {
        return { false, QStringLiteral("[Derrotas] Error: no se pudo crear el archivo de log en disco.") };
    }

    p.recordingActive     = true;
    p.recordingTrackId    = trackId;
    p.recordingStartTime  = now;
    p.currentSegmentIndex = 0;
    p.currentLogFileName  = m_logManager.currentFileName();

    return { true, QStringLiteral("[Derrotas] Grabacion iniciada para el track %1.").arg(trackId) };
}

DerrotasOperationResult DerrotasService::stopRecording()
{
    DerrotasPasadaState& p = m_ctx->derrotasSession.pasada;

    if (!p.recordingActive) {
        return { false, QStringLiteral("[Derrotas] No hay ninguna grabacion activa.") };
    }

    m_logManager.closeLog();
    p.reset();

    return { true, QStringLiteral("[Derrotas] Grabacion finalizada.") };
}

// ── BORRAR ──────────────────────────────────────────────────

DerrotasOperationResult DerrotasService::clearAll()
{
    DerrotasSessionState& s = m_ctx->derrotasSession;
    const bool anyActive = s.futura.active || s.pasada.recordingActive;

    if (s.pasada.recordingActive) {
        m_logManager.closeLog();
    }

    s.reset();

    if (!anyActive) {
        return { false, QStringLiteral("[Derrotas] No hay asesoramientos ni grabaciones activas para borrar.") };
    }
    return { true, QStringLiteral("[Derrotas] Asesoramientos futuros y grabacion finalizados.") };
}

// ── Tick (80ms) ──────────────────────────────────────────────────────────

void DerrotasService::update()
{
    const QDateTime now = QDateTime::currentDateTime();

    // Derrotas Futuras
    DerrotasFuturaState& f = m_ctx->derrotasSession.futura;
    if (f.active) {
        const Track* track = m_ctx->findTrackById(f.config.trackId);
        if (track) {
            const double courseDeg = track->getCourseDeg();
            const double speedKn   = track->getVelocidadDmPerHour() * Track::kDmToNm;

            QList<QPointF> crossPoints;
            double totalDistanceDm = 0.0;
            FuturaCalculator::calculateProjection(
                QPointF(track->getX(), track->getY()), courseDeg, speedKn, f.config.timeMinutes,
                crossPoints, totalDistanceDm);

            f.crossPoints     = crossPoints;
            f.totalDistanceDm = totalDistanceDm;

            f.alarmTriggered = FuturaCalculator::evaluateAlarm(
                f.baseCourseDeg, f.baseSpeedKn, courseDeg, speedKn,
                f.config.thresholdDeg, f.config.thresholdKn);
        }
    }

    // Derrotas Pasadas
    DerrotasPasadaState& p = m_ctx->derrotasSession.pasada;
    if (p.recordingActive) {
        // F4: corte automatico a las 24hs de grabacion continua.
        if (m_logManager.exceededMaxDuration(now)) {
            m_logManager.closeLog();
            p.reset();
            return;
        }

        const Track* track = m_ctx->findTrackById(p.recordingTrackId);
        if (track) {
            if (!m_ctx->ownShip.valid) {
                m_ctx->out << QStringLiteral("[Derrotas] Advertencia: el Buque Propio no tiene geolocalizacion valida. Punto omitido en el log.\n");
                m_ctx->out.flush();
            } else {
                double latDeg = 0.0, lonDeg = 0.0;
                RadarMath::dmToLatLon(
                    m_ctx->ownShip.latitudeDeg, m_ctx->ownShip.longitudeDeg,
                    track->getX(), track->getY(),
                    latDeg, lonDeg);

                DerrotasLogPoint point;
                point.trackName = QStringLiteral("tn%1").arg(p.recordingTrackId, 4, 10, QChar('0'));
                point.timestamp = now;
                point.latDeg    = latDeg;
                point.lonDeg    = lonDeg;
                point.rvDeg     = track->getCourseDeg();
                point.vdKn      = track->getVelocidadDmPerHour() * Track::kDmToNm;

                m_logManager.writePoint(point);
            }
        }

        if (m_logManager.rotateIfNeeded(now)) {
            p.currentSegmentIndex++;
            p.currentLogFileName = m_logManager.currentFileName();
        }
    }
}