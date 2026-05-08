#include "listcursorscommand.h"
#include <QString>

CommandResult ListCursorsCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    Q_UNUSED(inv);

    const auto& cursors = ctx.cursors; // público en CommandContext
    if (cursors.empty()) return {true, "(sin cursores)"};  // texto simple como en list

    QString out;
    out += "ID  | TYPE | ANGLE |  LONG |      X   |      Y   | ACTIVE\n";
    out += "----+------+-------+-------+----------+----------+--------\n";

    for (const CursorEntity& c : cursors) {
        // ---- ADAPTA ESTOS GETTERS A TU API DE CursorEntity ----
        const int id      = c.getCursorId();            // <-- si es id() o similar, ajusta aquí
        const int type    = c.getLineType();      // <-- si es type(), ajusta
        const double ang  = static_cast<double>(c.getCursorAngle()); // angle()
        const double lon  = static_cast<double>(c.getCursorLength());  // length() / long()
        const QPair<float,float> coord = c.getCoordinates();         // coord() o position()
        const bool active  = c.isActive();        // si no existe, pon siempre false
        // --------------------------------------------------------

        out += QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                   .arg(id, 3)
                   .arg(QString::number(type).rightJustified(4, ' '))
                   .arg(QString::number(ang, 'f', 1).rightJustified(5, ' '))
                   .arg(QString::number(lon, 'f', 1).rightJustified(5, ' '))
                   .arg(QString::number(coord.first,  'f', 3).rightJustified(8, ' '))
                   .arg(QString::number(coord.second, 'f', 3).rightJustified(8, ' '))
                   .arg(active ? "true   " : "false  ");
    }

    out.chop(1); // quitar el último '\n' como en list
    return {true, out};
}
