#include "trackservice.h"

#include "commandContext.h"
#include "entities/track.h"
#include "trackpppservice.h"
#include <QJsonObject>

TrackService::TrackService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

TrackOperationResult TrackService::createTrack(const TrackCreateRequest& request)
{
    if (request.x < -256.0 || request.x > 256.0 || request.y < -256.0 || request.y > 256.0) {
        return {false, "INVALID_COORDINATES", "Coordenadas fuera de rango. Deben estar entre -256 y 256.", -1};
    }

    if (request.speedDmPerHour.has_value()) {
        if (request.speedDmPerHour.value() < 0.0 || request.speedDmPerHour.value() > 99.9) {
            return {false, "INVALID_SPEED", "Velocidad DM/h fuera de rango (0..99.9)", -1};
        }
    }

    if (request.courseDeg.has_value()) {
        if (request.courseDeg.value() < 0 || request.courseDeg.value() > 359) {
            return {false, "INVALID_COURSE", "Curso fuera de rango (0..359)", -1};
        }
    }

    if (request.fc.has_value()) {
        if (request.fc.value() < 1 || request.fc.value() > 6) {
            return {false, "INVALID_FC", "Asignacion FC invalida (1..6)", -1};
        }
    }

    const int id = m_context->nextTrackId++;

    Track& track = m_context->emplaceTrackFront(
        id,
        request.type,
        request.identity,
        request.mode,
        static_cast<float>(request.x),
        static_cast<float>(request.y),
        request.ctorSpeedKnots,
        request.ctorCourseDeg,
        request.creationEnvironment.value_or(request.type)
    );

    if (request.speedDmPerHour.has_value()) track.setVelocidadDmPerHour(request.speedDmPerHour.value());
    if (request.courseDeg.has_value()) track.setCursoInt(request.courseDeg.value());
    if (request.fc.has_value()) track.setAsignacionFc(request.fc.value());
    if (request.asgc.has_value()) track.setCodigoAsignacion(request.asgc.value());
    if (request.linkY.has_value()) track.setEstadoLinkY(request.linkY.value());
    if (request.link14.has_value()) track.setEstadoLink14(request.link14.value());
    if (request.info.has_value()) {
        track.setInformacionAmpliatoria(request.info.value());
        m_context->setSitrepInfo(id, request.info.value());
    }
    if (request.priv.has_value()) track.setCodigoPrivado(request.priv.value());

    // Solo calculamos PPP al crear track cuando OwnShip ya fue seteado.
    // No hay recalc periodico en esta etapa porque los tracks se tratan como estaticos.
    if (m_context->ownShip.valid) {
        TrackPppService(m_context).recalculateTrackAgainstOwnShip(track);
    }

    return {true, QString(), QString(), id};
}

TrackOperationResult TrackService::deleteTrackById(int trackId)
{
    if (trackId < 0) {
        return {false, "INVALID_ID", "El id del track debe ser no negativo", -1};
    }

    if (!m_context->eraseTrackById(trackId)) {
        return {false, "NOT_FOUND", "Track no encontrado", trackId};
    }

    return {true, QString(), QString(), trackId};
}

Track* TrackService::findTrackById(int trackId) const
{
    if (!m_context) return nullptr;
    return m_context->findTrackById(trackId);
}

QJsonArray TrackService::serializeTracks() const
{
    auto identityToString = [](const Track& t) -> QString {
        return TrackData::toQString(t.getIdentity());
    };

    QJsonArray arr;
    for (const Track& tr : m_context->getTracks()) {
        const Track::SitrepPppData ppp = tr.getSitrepPpp();

        QString pppStatus = QStringLiteral("not_computed");
        if (ppp.status == Track::SitrepPppData::NoOwnShip) {
            pppStatus = QStringLiteral("no_ownship");
        } else if (ppp.status == Track::SitrepPppData::DegenerateRelativeMotion) {
            pppStatus = QStringLiteral("degenerate_relative_motion");
        } else if (ppp.status == Track::SitrepPppData::Valid) {
            pppStatus = QStringLiteral("valid");
        }

        QJsonObject trackObj;
        trackObj["id"] = tr.getId();
        trackObj["type"] = TrackData::toQString(tr.getType());
        trackObj["type_bits"] = TrackData::typeBits(tr.getType());
        trackObj["creation_environment"] = TrackData::toQString(tr.getCreationEnvironment());
        trackObj["creation_environment_bits"] = TrackData::typeBits(tr.getCreationEnvironment());
        trackObj["identity"] = identityToString(tr);
        trackObj["azimut"] = tr.getAzimuthDeg();
        trackObj["distancia"] = tr.getDistanceDm();
        trackObj["rumbo"] = tr.getCursoInt();
        trackObj["velocidad"] = tr.getVelocidadDmPerHour();
        trackObj["link"] = tr.getEstadoLinkY() == Track::LinkY_Invalid ? "--" :
                           QString(QChar("RCTS"[int(tr.getEstadoLinkY())]));
        trackObj["lat"] = tr.getY();
        trackObj["lon"] = tr.getX();
        trackObj["info"] = tr.getInformacionAmpliatoria();
        trackObj["ppp_az"] = ppp.azDeg;
        trackObj["ppp_dt"] = ppp.distanceDm;
        trackObj["ppp_t_hhmm"] = tr.getSitrepPppTimeHHMM();
        trackObj["ppp_status"] = pppStatus;
        trackObj["ppp_reason"] = ppp.reason;
        arr.append(trackObj);
    }
    
    return arr;
}
