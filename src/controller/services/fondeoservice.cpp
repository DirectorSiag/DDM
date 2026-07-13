#include "fondeoservice.h"
#include "model/fondeo/fondeoCalculator.h"

FondeoService::FondeoService(CommandContext* ctx)
    : m_ctx(ctx)
{}

FondeoOperationResult FondeoService::startSession(const FondeoConfig& config)
{
    if (!config.useTrack && !config.useGms) {
        return { false,QStringLiteral("Error: Se debe especificar explícitamente el modo de operación (useTrack = true o useGms = true).")};
    }

    if (config.useTrack && config.useGms) {
        return { false,QStringLiteral("Error: Se debe especificar un solo modo de operación (useTrack = true o useGms = true).")};
    }

    if (config.paAz < 0.0 || config.paAz >= 360.0) {
        return { false,QStringLiteral("Error: El azimut al Punto Auxiliar debe estar entre 0 y 359 grados.")};
    }
    if (config.paDt <= 0.0) {
        return { false,QStringLiteral("Error: La distancia al Punto Auxiliar debe ser mayor a cero.")};
    }

    if (!(config.r1 > config.r2 && config.r2 > config.r3 &&
          config.r3 > config.r4 && config.r4 > config.r5 && config.r5 > 0)) {
        return { false,QStringLiteral("Error: Los radios deben ser estrictamente decrecientes (R1 > R2 > R3 > R4 > R5) y mayores a cero.")};
    }

    QPointF trackPos(0.0, 0.0);
    double ownLatDeg = 0.0;
    double ownLonDeg = 0.0;
    QPointF puntoFondeo;

    if (config.useTrack) {
        // --- MODO TRACK DE REFERENCIA ---
        const Track* trackRef = m_ctx->findTrackById(config.trackId);
        if (!trackRef) {
            return { false, QStringLiteral("Error: El Track de referencia %1 no existe.").arg(config.trackId)};
        }
        trackPos = QPointF(trackRef->getX(), trackRef->getY());
        puntoFondeo = FondeoCalculator::resolvePuntoFondeo(config, trackPos);
    } else {
        // --- MODO GMS ---
        const bool bpHasGeo = m_ctx->ownShip.valid &&
                              !(m_ctx->ownShip.latitudeDeg == 0.0 && m_ctx->ownShip.longitudeDeg == 0.0);

        if (!bpHasGeo) {
            return { false, QStringLiteral("Error: El Buque Propio no tiene coordenadas geográficas válidas para usar el modo GMS.") };
        }

        ownLatDeg = m_ctx->ownShip.latitudeDeg;
        ownLonDeg = m_ctx->ownShip.longitudeDeg;
        puntoFondeo = FondeoCalculator::resolvePuntoFondeo(config, ownLatDeg, ownLonDeg);
    }

    QPointF puntoAuxiliar = FondeoCalculator::resolvePuntoAuxiliar(puntoFondeo, config.paAz, config.paDt);

    m_ctx->fondeoSession.reset();
    m_ctx->fondeoSession.config = config;
    m_ctx->fondeoSession.puntoFondeo = puntoFondeo;
    m_ctx->fondeoSession.puntoAuxiliar = puntoAuxiliar;
    m_ctx->fondeoSession.active = true;
    m_ctx->fondeoSession.paAlcanzado = false;

    return { true, QStringLiteral("[Fondeo] Maniobra de fondeo iniciada con éxito.") };
}

FondeoOperationResult FondeoService::stopSession()
{
    if (!m_ctx->fondeoSession.active) {
        return { false, QStringLiteral("[Fondeo] No hay ninguna maniobra de fondeo activa en este momento.") };
    }
    m_ctx->fondeoSession.reset();
    return { true, QStringLiteral("[Fondeo] Maniobra de fondeo finalizada.") };
}

void FondeoService::update()
{
    if (!m_ctx->fondeoSession.active) return;

    FondeoSessionState& s = m_ctx->fondeoSession;

    // Buque propio: track 0
    QPointF ownPos(0.0, 0.0);
    double ownCourse = 0.0;
    const Track* ownTrack = m_ctx->findTrackById(0);
    if (ownTrack) {
        ownPos = QPointF(ownTrack->getX(), ownTrack->getY());
        ownCourse = ownTrack->getCourseDeg();
    }

    FondeoCalculator::calculateDistAzPfPa(ownPos, s);

    if (!s.paAlcanzado && s.distanciaPA <= 50.0){ // 50 Yardas, valor de prueba
        m_ctx->out << QStringLiteral("\n[Fondeo] Se ha alcanzado el Punto Auxiliar.\n");
        s.paAlcanzado = true;
    }
    if (s.paAlcanzado && s.distanciaPF <= 15.0) { // 15 Yardas, valor de prueba
        m_ctx->out << QStringLiteral("\n[Fondeo] Se ha alcanzado el Punto de Fondeo. Finalizando cálculo cinemático.\n");
        m_ctx->out.flush();
        stopSession();
        return;
    }

    FondeoCalculator::calculateMarcacionRelativa(ownCourse, s);
    FondeoCalculator::calculatePanelPredictivo(s);
}