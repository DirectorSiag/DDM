#include "sitrepservice.h"
#include "../../model/commandContext.h"

#include "../../model/entities/track.h"
#include "trackservice.h"

SitrepService::SitrepService(CommandContext* context)
    : m_context(context)
{
}

std::deque<Track> SitrepService::snapshot() const {
    return m_context->getTracks();
}

bool SitrepService::deleteTrackById(int id) {
    // Puerta única: la baja pasa por TrackService (publica a replicación).
    const bool erased = m_context->trackService->deleteTrackById(id).success;
    if (erased) {
        // mantener compatibilidad: limpiar el mapa sitrepExtra asociado
        m_context->sitrepExtra.erase(id);
    }
    return erased;
}

Track* SitrepService::findTrackById(int id) const {
    return m_context->findTrackById(id);
}

void SitrepService::setSitrepInfo(int id, const QString& text) {
    m_context->setSitrepInfo(id, text);
}
