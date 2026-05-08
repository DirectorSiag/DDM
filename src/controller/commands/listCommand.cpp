/*
    Comando `list`/`ls`: imprime tabla con `id`, `type`, `ident`, `x`, `y` de los tracks existentes.
*/
#include "listCommand.h"
#include "enums.h"         // Q_NAMESPACE + enum class Type {...}
#include "../services/trackservice.h"
#include <QJsonArray>
#include <QJsonObject>

CommandResult ListCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    Q_UNUSED(inv);

    TrackService trackService(&ctx);
    const QJsonArray serializedTracks = trackService.serializeTracks();
    if (serializedTracks.isEmpty()) return {true, "(sin tracks)"};

    QString out;
    out += "ID  | TYPE     | IDENT |      X   |      Y\n";
    out += "----+----------+-------+----------+----------\n";

    for (const QJsonValue& value : serializedTracks) {
        const QJsonObject track = value.toObject();
        const int trackId = track.value("id").toInt();
        const QString trackIdOctal = QString::number(trackId, 8).rightJustified(4, '0');

        out += QString("%1 | %2 | %3 | %4 | %5\n")
                   .arg(trackIdOctal)
                   .arg(track.value("type").toString().left(8).leftJustified(8, ' '))
                   .arg(track.value("identity").toString().left(5).leftJustified(5, ' '))
                   .arg(QString::number(track.value("lon").toDouble(), 'f', 3).rightJustified(8, ' '))
                   .arg(QString::number(track.value("lat").toDouble(), 'f', 3).rightJustified(8, ' '));
    }

    out.chop(1); // quitar el último '\n'
    return {true, out};
}

