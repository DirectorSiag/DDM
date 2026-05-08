#include "queryservice.h"
#include "commandContext.h"
#include "entities/track.h"

QueryService::QueryService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

std::deque<Track> QueryService::findTracksByType(const QString& type) const
{
    std::deque<Track> result;
    if (!m_context) return result;

    for (const Track& track : m_context->getTracks()) {
        if (TrackData::toQString(track.getType()) == type) {
            result.push_back(track);
        }
    }
    return result;
}

std::deque<Track> QueryService::findTracksByIdentity(const QString& identity) const
{
    std::deque<Track> result;
    if (!m_context) return result;

    for (const Track& track : m_context->getTracks()) {
        if (TrackData::toQString(track.getIdentity()) == identity) {
            result.push_back(track);
        }
    }
    return result;
}

std::deque<Track> QueryService::findAllTracks() const
{
    if (!m_context) return {};
    return m_context->getTracks();
}

int QueryService::countTracks() const
{
    if (!m_context) return 0;
    return static_cast<int>(m_context->getTracks().size());
}

int QueryService::countCursors() const
{
    if (!m_context) return 0;
    return static_cast<int>(m_context->getCursors().size());
}
