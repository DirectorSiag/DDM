#include "displaymodecommand.h"

namespace {
QString modeToString(CommandContext::MotionMode mode)
{
    return mode == CommandContext::TRUE_MOTION
        ? QStringLiteral("true_motion")
        : QStringLiteral("relative");
}
}

CommandResult DisplayModeCommand::execute(const CommandInvocation& inv, CommandContext& ctx) const
{
    if (inv.args.isEmpty()) {
        return {false, QStringLiteral("Faltan argumentos. Uso: %1").arg(usage())};
    }

    const QString section = inv.args[0].trimmed().toLower();
    if (section != QStringLiteral("mode")) {
        return {false, QStringLiteral("Seccion invalida '%1'. Uso: %2").arg(section, usage())};
    }

    if (inv.args.size() == 1 || inv.args[1].trimmed().toLower() == QStringLiteral("show")) {
        return {true, QStringLiteral("Display mode actual: %1").arg(modeToString(ctx.getMotionMode()))};
    }

    const QString modeArg = inv.args[1].trimmed().toLower();
    if (modeArg == QStringLiteral("relative") || modeArg == QStringLiteral("rm")) {
        ctx.setMotionMode(CommandContext::RELATIVE);
        return {true, QStringLiteral("Display mode cambiado a relative")};
    }

    if (modeArg == QStringLiteral("true")
        || modeArg == QStringLiteral("true_motion")
        || modeArg == QStringLiteral("tm")) {
        ctx.setMotionMode(CommandContext::TRUE_MOTION);
        return {true, QStringLiteral("Display mode cambiado a true_motion")};
    }

    return {false, QStringLiteral("Modo invalido '%1'. Uso: %2").arg(modeArg, usage())};
}
