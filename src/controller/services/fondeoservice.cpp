#include "fondeoservice.h"
#include "geometryservice.h"
#include "model/fondeo/fondeoCalculator.h"
#include "RadarMath.h"

namespace {
// Tipos/colores de trabajo para las figuras de Fondeo. El "type" (0-7) es lo
// unico que viaja al protocolo binario LPD (3 bits de lineType); 4/5 no
// colisionan con los 1/2/3 que 2W reserva para guia/propio/aliadas. El campo
// "color" solo es visible para un consumidor JSON de list_shapes; el mapeo
// real tipo->color en el renderer queda pendiente (ver
// docs/modules/planFondeo.md, seccion "Puntos abiertos").
constexpr int kAnilloCircleType = 4;
constexpr int kPaCircleType     = 5;

const QString kAnilloColor = QStringLiteral("#00FF00"); // spec UI: "Color line: green"
const QString kPaColor     = QStringLiteral("#FF00FF");

// Umbral de llegada al PA en yardas (valor de prueba, pendiente de doctrina).
// Se comparte entre la transicion de fase de update() y el radio del circulo
// del PA, para que figura y umbral no puedan divergir.
constexpr double kPaArrivalYds = 50.0;
}

FondeoService::FondeoService(CommandContext* ctx)
    : m_ctx(ctx)
{}

FondeoOperationResult FondeoService::startSession(const FondeoConfig& config)
{
    // Sin este guard, un segundo start haria reset() implicito pisando los IDs
    // de figuras sin borrarlas -> circulos y cursores huerfanos en el radar.
    if (m_ctx->fondeoSession.active) {
        return { false, QStringLiteral("Error: Ya hay una maniobra de fondeo activa. Ejecute fondeo --stop primero.") };
    }

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

    // PF/PA y radios son estaticos durante toda la sesion, asi que las figuras
    // se publican una unica vez aca (y se borran en stopSession); no hay
    // reposicionamiento por tick como en 2W (ver docs/modules/planFondeo.md).
    createFigures();

    return { true, QStringLiteral("[Fondeo] Maniobra de fondeo iniciada con éxito.") };
}

FondeoOperationResult FondeoService::stopSession()
{
    if (!m_ctx->fondeoSession.active) {
        return { false, QStringLiteral("[Fondeo] No hay ninguna maniobra de fondeo activa en este momento.") };
    }
    deleteFigures();
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

    if (!s.paAlcanzado && s.distanciaPA <= kPaArrivalYds){
        m_ctx->out << QStringLiteral("\n[Fondeo] Se ha alcanzado el Punto Auxiliar.\n");
        s.paAlcanzado = true;

        // Fin de la Fase 1: el circulo del PA ya cumplio su funcion de guia.
        // Unico evento grafico entre el inicio y el fin de la sesion.
        if (s.paCircleId != FondeoSessionState::NO_CIRCLE) {
            GeometryService geometry(m_ctx);
            geometry.deleteCircle(s.paCircleId);
            s.paCircleId = FondeoSessionState::NO_CIRCLE;
        }
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

void FondeoService::createFigures()
{
    FondeoSessionState& s = m_ctx->fondeoSession;
    GeometryService geometry(m_ctx);

    // Anillos de marcha: concentricos en el PF, radios r1..r5 (yardas -> DM).
    // La validacion r1 > ... > r5 > 0 de startSession garantiza radios validos.
    const double radiosYds[] = { s.config.r1, s.config.r2, s.config.r3, s.config.r4, s.config.r5 };
    for (double rYds : radiosYds) {
        const GeometryResult r = geometry.createCircle(
            s.puntoFondeo, RadarMath::yardsToDm(rYds), kAnilloCircleType, kAnilloColor);
        s.anillosCircleIds.append(r.success ? r.id : FondeoSessionState::NO_CIRCLE);
    }

    const GeometryResult rPa = geometry.createCircle(
        s.puntoAuxiliar, RadarMath::yardsToDm(kPaArrivalYds), kPaCircleType, kPaColor);
    s.paCircleId = rPa.success ? rPa.id : FondeoSessionState::NO_CIRCLE;
}

void FondeoService::deleteFigures()
{
    FondeoSessionState& s = m_ctx->fondeoSession;
    GeometryService geometry(m_ctx);

    for (int id : s.anillosCircleIds) {
        if (id != FondeoSessionState::NO_CIRCLE) geometry.deleteCircle(id);
    }
    s.anillosCircleIds.clear();

    if (s.paCircleId != FondeoSessionState::NO_CIRCLE) {
        geometry.deleteCircle(s.paCircleId);
        s.paCircleId = FondeoSessionState::NO_CIRCLE;
    }
}