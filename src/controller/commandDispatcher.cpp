/*
    Interfaz/implementación del despachador: recibe líneas, invoca el parser y
    ejecuta el comando correspondiente; maneja `help` y `exit`.
*/
#include "CommandDispatcher.h"
#include "commandRegistry.h"
#include "iInputParser.h"
#include "iCommand.h"
#include "commandContext.h"
#include <QStringList>
#include "consoleUtils.h"
#include "ansi.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <QRegularExpression>
#endif


CommandDispatcher::CommandDispatcher(CommandRegistry* reg,
                                     IInputParser* parser,
                                     CommandContext& ctx,
                                     QObject* parent)
    : QObject(parent), reg_(reg), parser_(parser), ctx_(ctx) {}



void CommandDispatcher::onLine(const QString& line) {
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) return;

    Console::clear(ctx_.out);

    auto printPrefijo = [&](bool ok) {
        const char* color = ok ? Ansi::green : Ansi::red;
        ctx_.out << color << ctx_.commandCounter << " " << trimmed << Ansi::reset << "\n";
        ctx_.out.flush();
        ctx_.commandCounter++;
    };


    // Intento parsear
    CommandInvocation inv;
    QString perr;
    if (!parser_->parse(trimmed, inv, perr)) {
        printPrefijo(false);
        ctx_.err << Ansi::red
                 << "Error de parseo: " << perr << "\n"
                 << "Use 'help' para ver la lista de comandos."
                 << Ansi::reset << "\n";
        ctx_.err.flush();
        return;
    }

    const QString first = inv.name;

    // Admin: exit/salir
    if (first.compare("salir", Qt::CaseInsensitive) == 0
        || first.compare("exit", Qt::CaseInsensitive) == 0) {
        printPrefijo(true);
        ctx_.out << "Adiós!\n"; ctx_.out.flush();
        emit quitRequested();
        return;
    }

    // Admin: help
    if (first.compare("help", Qt::CaseInsensitive) == 0) {
        printPrefijo(true);
        ctx_.out << Ansi::cyan << "Comandos:\n";

        // Admin fijos
        ctx_.out << "  help, exit/salir\n";

        for (const auto& sp : reg_->all()) {
            const ICommand* cmd = sp.data();
            ctx_.out << "  "
                     << cmd->usage()
                     << "  - " << cmd->getDescription()
                     << "\n";
        }

        ctx_.out << Ansi::reset;
        ctx_.out.flush();
        return;
    }

    // Lookup de comando real
    auto cmd = reg_->find(inv.name);
    if (cmd.isNull()) {
        printPrefijo(false);
        ctx_.err << Ansi::red
                 << "Comando desconocido: " << inv.name << "\n"
                 << "Use 'help' para ver la lista de comandos."
                 << Ansi::reset << "\n";
        ctx_.err.flush();
        return;
    }

    // Ejecutar
    auto res = cmd->execute(inv, ctx_);
    printPrefijo(res.ok);

    if (!res.message.isEmpty()) {
        QTextStream& ts = res.ok ? ctx_.out : ctx_.err;
        const char* color = res.ok ? Ansi::green : Ansi::red;
        ts << color << res.message << Ansi::reset << "\n";
        ts.flush();
        if (!res.ok) {
            ctx_.err << "Use 'help' para ver la lista de comandos.\n";
            ctx_.err.flush();
        }
    }
}
