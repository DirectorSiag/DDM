#pragma once

#include "entities/track.h"
#include "../services/trackservice.h"
#include <QJsonObject>
#include <QByteArray>
#include <QString>

class CommandContext;
class ITransport;

/**
 * @brief Manejador de comandos para tracks (sitrep)
 */
class TrackCommandHandler
{
public:
    TrackCommandHandler(CommandContext* context, ITransport* transport);

    QByteArray createTrack(const QJsonObject& args);
    QByteArray deleteTrack(const QJsonObject& args);
    QByteArray listTracks(const QJsonObject& args);

private:
    CommandContext* m_context;
    ITransport* m_transport;

    QByteArray buildCreateTrackResponse(int createdId);
    QByteArray buildDeleteTrackResponse(int deletedId);
    QByteArray buildListTracksResponse();
};
