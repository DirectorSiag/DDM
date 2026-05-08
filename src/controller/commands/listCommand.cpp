/*
    Comando `list`/`ls`: imprime tabla con `id`, `type`, `ident`, `x`, `y` de los tracks existentes.
*/
#include "Commands/listCommand.h"
#include "enums.h"         // Q_NAMESPACE + enum class Type {...}

CommandResult ListCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    Q_UNUSED(inv);

    const auto& tracks = ctx.getTracks();
    if (tracks.empty()) return {true, "(sin tracks)"};

    QString out;
    out += "ID  | TYPE     | IDENT |      X   |      Y\n";
    out += "----+----------+-------+----------+----------\n";

    for (const Track& track : tracks) {  // más nuevos primero (push_front)
        out += QString("%1 | %2 | %3 | %4 | %5\n")
                   .arg(track.getId(), 3)
                   .arg(TrackData::toQString(track.getType()).left(8).leftJustified(8, ' '))
                   .arg(TrackData::toQString(track.getIdentity()).left(5).leftJustified(5, ' '))
                   .arg(QString::number(track.getX(), 'f', 3).rightJustified(8, ' '))
                   .arg(QString::number(track.getY(), 'f', 3).rightJustified(8, ' '));
    }

    out.chop(1); // quitar el último '\n'
    return {true, out};
}

