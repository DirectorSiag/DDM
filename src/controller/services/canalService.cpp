#include "canalService.h"
#include "model/canal/canalCalculator.h"
#include "entities/track.h"

CanalService::CanalService(CommandContext* ctx) : m_ctx(ctx) {}

CanalOperationResult CanalService::startSession(const CanalConfig& config) {
    // 1. Validar que los tracks existan
    auto validateTrack = [&](bool isSet, int trackId, const QString& colName) -> QString {
        if (!isSet) return "";
        if (!m_ctx->findTrackById(trackId)) {
            return QStringLiteral("El Track %1 para la columna %2 no existe.").arg(trackId).arg(colName);
        }
        return "";
    };

    QString err = validateTrack(config.setA, config.trackA, "A");
    if (err.isEmpty()) err = validateTrack(config.setB, config.trackB, "B");
    if (err.isEmpty()) err = validateTrack(config.setC, config.trackC, "C");
    if (err.isEmpty()) err = validateTrack(config.setD, config.trackD, "D");

    if (!err.isEmpty()) {
        return {false, err}; // Si un track es inválido falla
    }

    // 2. Validar que no se estén sobreescribiendo columnas activas
    auto isBusy = [&](bool isSet, int index) {
        return isSet && m_ctx->canalSession.columnas[index].active;
    };

    if (isBusy(config.setA, 0) || isBusy(config.setB, 1) ||
        isBusy(config.setC, 2) || isBusy(config.setD, 3)) {
        return {false, QStringLiteral("No se pueden sobreescribir columnas en uso. Espere a que se liberen o use BORRAR.")};
    }

    // 3. Aplicar los cambios al estado
    m_ctx->canalSession.active = true; // Iniciamos la sesión global

    if (config.setA) { m_ctx->canalSession.columnas[0].active = true; m_ctx->canalSession.columnas[0].trackId = config.trackA; }
    if (config.setB) { m_ctx->canalSession.columnas[1].active = true; m_ctx->canalSession.columnas[1].trackId = config.trackB; }
    if (config.setC) { m_ctx->canalSession.columnas[2].active = true; m_ctx->canalSession.columnas[2].trackId = config.trackC; }
    if (config.setD) { m_ctx->canalSession.columnas[3].active = true; m_ctx->canalSession.columnas[3].trackId = config.trackD; }

    return {true, QStringLiteral("[Canal] Asesoramiento iniciado correctamente.")};
}

CanalOperationResult CanalService::stopSession() {
    if (!m_ctx->canalSession.active) {
        return {false, QStringLiteral("[Canal] No hay ningún asesoramiento activo para borrar.")};
    }
    m_ctx->canalSession.reset();
    return {true, QStringLiteral("[Canal] Todas las columnas han sido borradas.")};
}

void CanalService::update() {
    // Si la sesión no fue iniciada globalmente, no consumimos recursos calculando nada
    if (!m_ctx->canalSession.active) return;

    // Obtenemos los datos cinemáticos del Buque Propio (Track 0)
    const Track* ownTrack = m_ctx->findTrackById(0);
    if (!ownTrack) return;

    QPointF ownPos(ownTrack->getX(), ownTrack->getY());
    double ownCourse = ownTrack->getCourseDeg();

    // Convertimos la velocidad de Nudos a Data Miles / hora para la matemática
    double ownSpeedDm = m_ctx->ownShip.speedKnots / Track::kDmToNm;

    bool anyColumnActive = false;

    // Iteramos sobre las 4 columnas (A, B, C y D)
    for (int i = 0; i < 4; i++) {
        CanalSlot& slot = m_ctx->canalSession.columnas[i];

        // Si la columna está vacía, pasamos a la siguiente
        if (!slot.active) continue;

        // Validamos que el Track de la boya siga existiendo
        const Track* buoyTrack = m_ctx->findTrackById(slot.trackId);
        if (!buoyTrack) {
            slot.reset(); // Si la boya se borró del sistema, limpiamos la columna
            continue;
        }

        // Si llegamos acá, sabemos que hay al menos una boya procesándose
        anyColumnActive = true;

        QPointF buoyPos(buoyTrack->getX(), buoyTrack->getY());

        // Cálculos cinemáticos puros (actualiza DT, AZ, RV, ETA y Marcación Relativa)
        CanalCalculator::calculateCinematica(ownPos, ownCourse, ownSpeedDm, buoyPos, slot);

        // Evaluamos la ventana de alarma (±5° del través)
        bool inWindow = CanalCalculator::isWithinAlarmWindow(slot.marcacionRelativa);

        if (inWindow) {
            slot.isAlarmActive = true;
            slot.hasTriggeredAlarm = true; // Activamos el seguro de memoria (entró en alarma)
        } else {
            slot.isAlarmActive = false;
        }

        // Reseteo: Se borra si sonó la alarma antes y la alarma ya se apagó
        if (slot.hasTriggeredAlarm && !inWindow) {
            // Verificamos si se apagó porque efectivamente la dejamos atrás
            if (CanalCalculator::isBehind(slot.marcacionRelativa)) {
                slot.reset();
            }
        }
    }

    // 4. Mantenimiento del estado global
    // Si el buque ya superó las 4 boyas y las 4 columnas se vaciaron solas, apagamos la sesión
    if (!anyColumnActive) {
        m_ctx->canalSession.active = false;
    }
}