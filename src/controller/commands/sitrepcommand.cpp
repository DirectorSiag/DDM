#include "Commands/sitrepCommand.h"

#include <QThread>
#include <QTextStream>
#include <QStringList>
#include <deque>
#include <cmath>   // std::isnan

namespace {

void clearScreen(QTextStream& out) {
    out << "\x1B[2J\x1B[H";
}

void printHeader(QTextStream& out, int tracksCount, int refreshMs) {
    out << "SITREP (refresh=" << refreshMs << "ms)  Tracks=" << tracksCount << "\n";
    out << "+-------+--------------+--------------+----------+--------+---------------------------+\n";
    out << "| Track | Identidad     | Az/Dt (DM)    | Spd (kt) | Crs(°) | Info Ampliatoria          |\n";
    out << "+-------+--------------+--------------+----------+--------+---------------------------+\n";
}

static QString fmtOrUnk(double v, int width, int prec) {
    if (std::isnan(v)) {
        return QString("%1").arg("UNK", width);
    }
    return QString("%1").arg(v, width, 'f', prec);
}

void printRow(QTextStream& out, const CommandContext& ctx, const Track& t) {
    const int id = t.getId();

    const QString ident = TrackData::toQString(t.getIdentity());
    const QString info  = ctx.sitrepInfo(id);

    const double azDeg  = t.getAzimuthDeg();
    const double distDm = t.getDistanceDm();

    const QString spdStr = fmtOrUnk(t.getSpeedKnots(), 8, 1);
    const QString crsStr = fmtOrUnk(t.getCourseDeg(),  6, 1);

    out
        << "| " << QString("%1").arg(id, 5)
        << " | " << QString("%1").arg(ident, 12)
        << " | " << QString("%1/%2")
                        .arg(azDeg, 6, 'f', 1)
                        .arg(distDm, 8, 'f', 2)
        << " | " << spdStr
        << " | " << crsStr
        << " | " << QString("%1").arg(info.left(25), 25)  // recorte para no romper tabla
        << " |\n";
}

void printFooter(QTextStream& out) {
    out << "+-------+--------------+--------------+----------+--------+---------------------------+\n";
    out << "\nCTRL+C para salir.\n";
    out.flush();
}

void printSitrep(QTextStream& out, const CommandContext& ctx, const std::deque<Track>& tracksSnap, int refreshMs) {
    printHeader(out, static_cast<int>(tracksSnap.size()), refreshMs);

    for (const Track& t : tracksSnap) {
        printRow(out, ctx, t);
    }

    printFooter(out);
}

} // namespace

CommandResult SitrepCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    const QStringList& args = inv.args;
    const QString sub = args.isEmpty() ? "list" : args[0].toLower();

    if (sub == "list") {
        const std::deque<Track> snap = ctx.getTracks();
        QString outMsg;
        QTextStream oss(&outMsg);
        static constexpr int REFRESH_MS = 2000;
        printSitrep(oss, ctx, snap, REFRESH_MS);
        return { true, outMsg };
    }

    if (sub == "watch") {
        static constexpr int REFRESH_MS = 2000;

        while (true) {
            const std::deque<Track> snap = ctx.getTracks();
            clearScreen(ctx.out);
            printSitrep(ctx.out, ctx, snap, REFRESH_MS);
            QThread::msleep(REFRESH_MS);
        }
        // no retorna (CTRL+C)
    }

    if (sub == "delete") {
        if (args.size() < 2) {
            return { false, "Uso: " + usage() };
        }

        bool ok = false;
        const int id = args[1].toInt(&ok);
        if (!ok) {
            return { false, "trackId invalido: " + args[1] };
        }

        const bool erased = ctx.eraseTrackById(id);
        if (!erased) {
            return { false, QString("No existe track con id=%1").arg(id) };
        }

        ctx.sitrepExtra.erase(id); // opcional
        return { true, QString("OK delete → id=%1").arg(id) };
    }

    if (sub == "info") {
        if (args.size() < 3) {
            return { false, "Uso: sitrep info <trackId> <texto>" };
        }

        bool ok = false;
        const int id = args[1].toInt(&ok);
        if (!ok) {
            return { false, "trackId invalido: " + args[1] };
        }

        if (!ctx.findTrackById(id)) {
            return { false, QString("No existe track con id=%1").arg(id) };
        }

        QString text = args[2];
        for (int i = 3; i < args.size(); ++i) {
            text += " ";
            text += args[i];
        }

        ctx.setSitrepInfo(id, text);
        return { true, QString("OK sitrep info → id=%1").arg(id) };
    }

    return { false, QString("Subcomando no soportado: %1\nUso: %2").arg(sub, usage()) };
}
