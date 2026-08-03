#include "borneoCommand.h"
#include "commandContext.h"
#include "../services/borneoService.h"
#include "../../model/borneo/borneoSessionState.h"

namespace {
QString extractOpt(const QStringList& args, const QString& key)
{
    const QString prefix = QStringLiteral("--%1=").arg(key);
    for (const QString& token : args) {
        if (token.startsWith(prefix)) {
            return token.mid(prefix.length());
        }
    }
    return QString();
}

bool hasFlag(const QStringList& args, const QString& key)
{
    const QString flag = QStringLiteral("--%1").arg(key);
    for (const QString& token : args) {
        if (token == flag || token.startsWith(flag + QStringLiteral("="))) {
            return true;
        }
    }
    return false;
}
}

QString BorneoCommand::getName() const
{
    return QStringLiteral("borneo");
}

QString BorneoCommand::getDescription() const
{
    return QStringLiteral("Calcula y gestiona el radio del Circulo de Borneo");
}

QString BorneoCommand::usage() const
{
    return QStringLiteral("borneo (--clase=<MEKO_360|MEKO_140|PATAGONIA> | --eslora=<mts>) --grilletes=<n> --profundidad=<mts> | --stop | --info");
}

CommandResult BorneoCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (hasFlag(inv.args, QStringLiteral("stop"))) {
        return handleFinalizar(ctx);
    }

    if (hasFlag(inv.args, QStringLiteral("info"))) {
        return handleInfo(ctx);
    }

    return handleIniciar(inv, ctx);
}

CommandResult BorneoCommand::handleIniciar(const CommandInvocation& inv, CommandContext& ctx) const
{
    const QString claseStr = extractOpt(inv.args, QStringLiteral("clase"));
    const QString esloraStr = extractOpt(inv.args, QStringLiteral("eslora"));
    const QString grilletesStr = extractOpt(inv.args, QStringLiteral("grilletes"));
    const QString profundidadStr = extractOpt(inv.args, QStringLiteral("profundidad"));

    const bool useClase = !claseStr.isEmpty();
    const bool useEslora = !esloraStr.isEmpty();

    if (useClase == useEslora) {
        // ambos presentes o ninguno -> error, son excluyentes
        return { false, QStringLiteral("Debe indicar exactamente uno: --clase=<...> o --eslora=<mts>") };
    }

    if (grilletesStr.isEmpty() || profundidadStr.isEmpty()) {
        return { false, QStringLiteral("Parametros requeridos: --grilletes --profundidad") };
    }

    bool okGrilletes = false, okProfundidad = false;
    const int grilletes = grilletesStr.toInt(&okGrilletes);
    const double profundidad = profundidadStr.toDouble(&okProfundidad);

    if (!okGrilletes || !okProfundidad) {
        return { false, QStringLiteral("Los valores de --grilletes y --profundidad deben ser numericos") };
    }

    BorneoService service(&ctx);

    if (useClase) {
        const BorneoOperationResult result = service.startSessionByClase(claseStr, grilletes, profundidad);
        return { result.success, result.message };
    }

    bool okEslora = false;
    const double eslora = esloraStr.toDouble(&okEslora);
    if (!okEslora) {
        return { false, QStringLiteral("El valor de --eslora debe ser numerico") };
    }

    BorneoConfig config;
    config.eslora = eslora;
    config.grilletes = grilletes;
    config.profundidad = profundidad;

    const BorneoOperationResult result = service.startSession(config);
    return { result.success, result.message };
}

CommandResult BorneoCommand::handleFinalizar(CommandContext& ctx) const
{
    BorneoService service(&ctx);
    const BorneoOperationResult result = service.stopSession();
    return { result.success, result.message };
}

CommandResult BorneoCommand::handleInfo(const CommandContext& ctx) const
{
    return { true, infoReport(ctx) };
}

QString BorneoCommand::infoReport(const CommandContext& ctx) const
{
    const BorneoSessionState& session = ctx.borneoSession;

    if (!session.active) {
        return QStringLiteral("Borneo: sin sesion activa");
    }

    return QStringLiteral("Borneo activo | Eslora: %1 mts | Grilletes: %2 | Profundidad: %3 mts | Radio: %4 mts")
        .arg(session.config.eslora, 0, 'f', 2)
        .arg(session.config.grilletes)
        .arg(session.config.profundidad, 0, 'f', 2)
        .arg(session.radioCalculado, 0, 'f', 2);
}