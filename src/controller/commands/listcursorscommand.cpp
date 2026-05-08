#include "listcursorscommand.h"
#include "../services/cursorservice.h"
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

CommandResult ListCursorsCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const {
    Q_UNUSED(inv);

    CursorService cs(&ctx);
    const QJsonArray serialized = cs.serializeCursors();
    if (serialized.isEmpty()) return {true, "(sin cursores)"};

    QString out;
    out += "ID  | TYPE | ANGLE |  LONG |      X   |      Y   | ACTIVE\n";
    out += "----+------+-------+-------+----------+----------+--------\n";

    for (const QJsonValue& v : serialized) {
        QJsonObject c = v.toObject();
        const int id = c.value("id").toInt();
        const int type = c.value("type").toInt();
        const double ang = c.value("angle").toDouble();
        const double lon = c.value("length").toDouble();
        const double x = c.value("x").toDouble();
        const double y = c.value("y").toDouble();
        const bool active = c.value("active").toBool();

        out += QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                   .arg(id, 3)
                   .arg(QString::number(type).rightJustified(4, ' '))
                   .arg(QString::number(ang, 'f', 1).rightJustified(5, ' '))
                   .arg(QString::number(lon, 'f', 1).rightJustified(5, ' '))
                   .arg(QString::number(x, 'f', 3).rightJustified(8, ' '))
                   .arg(QString::number(y, 'f', 3).rightJustified(8, ' '))
                   .arg(active ? "true   " : "false  ");
    }

    out.chop(1);
    return {true, out};
}
