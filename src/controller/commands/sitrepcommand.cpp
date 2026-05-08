#include "sitrepcommand.h"
#include "../services/sitrepservice.h"

#include <QThread>
#include <QTextStream>
#include <QStringList>
#include <deque>

namespace {

void clearScreen(QTextStream& out) {
    out << "\x1B[2J\x1B[H";
}

QString identidadUI(TrackData::Identity id) {
    // UI agrupa identidades:
    // Amigo  = PossFriend / ConfFriend / Heli
    // Hostil = PossHostile / ConfHostile
    // Desconocido = Pending / EvalUnknown (y default)
    switch (id) {
    case TrackData::PossFriend:
    case TrackData::ConfFriend:
    case TrackData::Heli:
        return "Amigo";
    case TrackData::PossHostile:
    case TrackData::ConfHostile:
        return "Hostil";
    case TrackData::Pending:
    case TrackData::EvalUnknown:
    default:
        return "Desconocido";
    }
}

QString linkUI(const Track& t) {
    // Regla simple estilo UI: RX / TX / --
    if (t.getEstadoLinkY() == Track::LinkY_ReceivedOnly) return "RX";
    if (t.getEstadoLinkY() == Track::LinkY_Tx || t.getEstadoLinkY() == Track::LinkY_TxS) return "TX";
    if (t.getEstadoLink14() == Track::Link14_Tx) return "TX";
    return "--";
}

QString latLongUI(const Track& /*t*/) {
    // Placeholder: aún no calculamos Lat/Long
    return "--";
}

QString fmtNum4(int n) {
    return QString("%1").arg(n, 4, 10, QLatin1Char('0'));
}

void printHeader(QTextStream& out, int tracksCount, int refreshMs, bool showCtrlC) {
    out << "SITREP (refresh=" << refreshMs << "ms)  Tracks=" << tracksCount << "\n";
    out << "+-------+-------------+-----------+-----------+------+---------------------+---------------------------+------------------+\n";
    out << "| Nro   | Identidad   | Az/Dt(DM) | Rv/Vd     | Link | Lat/Long            | Info Ampliatoria          | PPP Az/Dt/T      |\n";
    out << "+-------+-------------+-----------+-----------+------+---------------------+---------------------------+------------------+\n";
    if (!showCtrlC) out.flush();
}

void printRow(QTextStream& out, const Track& t) {
    const QString nro = fmtNum4(t.getNumero());

    const QString ident = QString("%1").arg(identidadUI(t.getIdentity()), 11);

    const QString azdt = QString("%1/%2")
                             .arg(t.getAzimuthDeg(), 3, 'f', 0)
                             .arg(t.getDistanceDm(), 4, 'f', 1);
    const QString azdtCell = QString("%1").arg(azdt, 9);

    // Rv/Vd (curso/velocidad). Velocidad en DM/h (modelo SURFACE TRACKS)
    const QString rvvd = QString("%1/%2")
                             .arg(t.getCursoInt(), 3, 10, QLatin1Char('0'))
                             .arg(t.getVelocidadDmPerHour(), 4, 'f', 1);
    const QString rvvdCell = QString("%1").arg(rvvd, 9);

    const QString link = QString("%1").arg(linkUI(t), 4);

    const QString ll = QString("%1").arg(latLongUI(t), 19);

    const QString info = t.getInformacionAmpliatoria().isEmpty() ? "-" : t.getInformacionAmpliatoria();
    const QString infoCell = QString("%1").arg(info.left(25), 25);

    const Track::SitrepPppData ppp = t.getSitrepPpp();
    QString pppCell = QStringLiteral("--/--/--:--");
    if (ppp.status == Track::SitrepPppData::Valid
        || ppp.status == Track::SitrepPppData::DegenerateRelativeMotion) {
        pppCell = QStringLiteral("%1/%2/%3")
                      .arg(ppp.azDeg, 3, 'f', 0)
                      .arg(ppp.distanceDm, 4, 'f', 1)
                      .arg(t.getSitrepPppTimeHHMM());
    }

    out
        << "| " << QString("%1").arg(nro, 5) << " "
        << "| " << QString("%1").arg(ident, 11) << " "
        << "| " << QString("%1").arg(azdtCell, 9) << " "
        << "| " << QString("%1").arg(rvvdCell, 9) << " "
        << "| " << QString("%1").arg(link, 4) << " "
        << "| " << QString("%1").arg(ll, 19) << " "
        << "| " << infoCell << " "
        << "| " << QString("%1").arg(pppCell, 16) << " |\n";
}

void printFooter(QTextStream& out, bool showCtrlC) {
    out << "+-------+-------------+-----------+-----------+------+---------------------+---------------------------+------------------+\n";
    if (showCtrlC) out << "\nCTRL+C para salir.\n";
    out.flush();
}

void printSitrep(QTextStream& out, const std::deque<Track>& tracksSnap, int refreshMs, bool showCtrlC) {
    printHeader(out, static_cast<int>(tracksSnap.size()), refreshMs, showCtrlC);
    for (const Track& t : tracksSnap) printRow(out, t);
    printFooter(out, showCtrlC);
}

} // namespace

CommandResult SitrepCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    const QStringList& args = inv.args;
    const QString sub = args.isEmpty() ? "list" : args[0].toLower();

    if (sub == "list") {
        SitrepService ss(&ctx);
        const std::deque<Track> snap = ss.snapshot();
        QString outMsg;
        QTextStream oss(&outMsg);
        static constexpr int REFRESH_MS = 2000;
        printSitrep(oss, snap, REFRESH_MS, /*showCtrlC*/false);
        return { true, outMsg };
    }

    if (sub == "watch") {
        static constexpr int REFRESH_MS = 2000;
        while (true) {
            SitrepService ss(&ctx);
            const std::deque<Track> snap = ss.snapshot();
            clearScreen(ctx.out);
            printSitrep(ctx.out, snap, REFRESH_MS, /*showCtrlC*/true);
            QThread::msleep(REFRESH_MS);
        }
        // no retorna (CTRL+C)
    }

    if (sub == "delete") {
        if (args.size() < 2) return { false, "Uso:\n" + usage() };

        bool ok = false;
        const int id = args[1].toInt(&ok);
        if (!ok) return { false, "trackId invalido: " + args[1] };

        SitrepService ss(&ctx);
        const bool erased = ss.deleteTrackById(id);
        if (!erased) return { false, QString("No existe track con id=%1").arg(id) };

        // compat viejo (si existe) - ya limpiado por SitrepService::deleteTrackById
        return { true, QString("OK delete → id=%1").arg(id) };
    }

    if (sub == "info") {
        if (args.size() < 3) return { false, "Uso: sitrep info <trackId> <texto...>" };

        bool ok = false;
        const int id = args[1].toInt(&ok);
        if (!ok) return { false, "trackId invalido: " + args[1] };

        SitrepService ss(&ctx);
        Track* t = ss.findTrackById(id);
        if (!t) return { false, QString("No existe track con id=%1").arg(id) };

        QString text = args[2];
        for (int i = 3; i < args.size(); ++i) {
            text += " ";
            text += args[i];
        }

        // Nuevo modelo: Info ampliatoria vive en el Track
        t->setInformacionAmpliatoria(text);

        // Compat: mantenemos el map viejo por si alguna otra parte lo usa
        ss.setSitrepInfo(id, text);

        return { true, QString("OK sitrep info → id=%1").arg(id) };
    }

    return { false, QString("Subcomando no soportado: %1\nUso:\n%2").arg(sub, usage()) };
}
