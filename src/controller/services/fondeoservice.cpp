#include "fondeoservice.h"
#include "model/fondeo/fondeoCalculator.h"

FondeoService::FondeoService(CommandContext* ctx)
    : m_ctx(ctx)
{}

bool FondeoService::startSession(const FondeoConfig& config, QString& outError)
{
    if (!config.useTrack && !config.useGms) {
        outError = QStringLiteral("Error: Se debe especificar explícitamente el modo de operación (useTrack = true o useGms = true).");
        return false;
    }

    if (config.useTrack && config.useGms) {
        outError = QStringLiteral("Error: Se debe especificar un solo modo de operación (useTrack = true o useGms = true).");
        return false;
    }

    if (config.paAz < 0.0 || config.paAz >= 360.0) {
        outError = QStringLiteral("Error: El azimut al Punto Auxiliar debe estar entre 0 y 359 grados.");
        return false;
    }
    if (config.paDt <= 0.0) {
        outError = QStringLiteral("Error: La distancia al Punto Auxiliar debe ser mayor a cero.");
        return false;
    }

    if (!(config.r1 > config.r2 && config.r2 > config.r3 &&
          config.r3 > config.r4 && config.r4 > config.r5 && config.r5 > 0)) {
        outError = QStringLiteral("Error: Los radios deben ser estrictamente decrecientes (R1 > R2 > R3 > R4 > R5) y mayores a cero.");
        return false;
    }

    QPointF trackPos(0.0, 0.0);
    double ownLatDec = 0.0;
    double ownLonDec = 0.0;

    if (config.useTrack) {
        const Track* trackRef = m_ctx->findTrackById(config.trackId);
        if (!trackRef) {
            outError = QStringLiteral("Error: El Track de referencia %1 no existe.").arg(config.trackId);
            return false;
        }
        trackPos = QPointF(trackRef->getX(), trackRef->getY());
    } else {
        const Track* ownTrack = m_ctx->findTrackById(0);
        if (!ownTrack) {
            outError = QStringLiteral("Error: No se encontro el Track 0 (Buque Propio) para referenciar el GMS.");
            return false;
        }

        // --- TODO: DESCOMENTAR CUANDO TRACK IMPLEMENTE GETTERS DE LAT/LON ---
        // ownLatDec = ownTrack->getLatitudDecimal();
        // ownLonDec = ownTrack->getLongitudDecimal();
    }

    QPointF puntoFondeo = FondeoCalculator::resolvePuntoFondeo(config, trackPos, ownLatDec, ownLonDec);

    QPointF puntoAuxiliar = FondeoCalculator::resolvePuntoAuxiliar(puntoFondeo, config.paAz, config.paDt);

    m_ctx->fondeoSession.reset();
    m_ctx->fondeoSession.config = config;
    m_ctx->fondeoSession.puntoFondeo = puntoFondeo;
    m_ctx->fondeoSession.puntoAuxiliar = puntoAuxiliar;
    m_ctx->fondeoSession.active = true;
    m_ctx->fondeoSession.paAlcanzado = false;

    m_ctx->out << QStringLiteral("\n[Fondeo] Maniobra de fondeo iniciada.\n");
    m_ctx->out.flush();

    return true;
}

void FondeoService::stopSession()
{
    m_ctx->fondeoSession.reset();
    m_ctx->out << QStringLiteral("\n[Fondeo] Maniobra de fondeo finalizada.\n");
    m_ctx->out.flush();
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

    if (!s.paAlcanzado && s.distanciaPA == 0.0) // Seguramente se tenga que modificar a <= Algún valor razonable
        s.paAlcanzado = true;
    if (s.paAlcanzado && s.distanciaPF == 0.0) { // Seguramente se tenga que modificar a <= Algún valor razonable
        m_ctx->out << QStringLiteral("\n[Fondeo] Se ha alcanzado el Punto de Fondeo. Finalizando cálculo cinemático.\n");
        m_ctx->out.flush();
        stopSession();
        return;
    }

    FondeoCalculator::calculateMarcacionRelativa(ownCourse, s);
    FondeoCalculator::calculatePanelPredictivo(s);
}