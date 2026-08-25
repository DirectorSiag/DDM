#include "borneoService.h"
#include "../../model/borneo/borneoSessionState.h"
#include "../../model/borneo/borneoCalculator.h"
#include "../../model/borneo/buqueClaseCatalog.h"
#include "commandContext.h"

#include <QDebug>

BorneoService::BorneoService(CommandContext* context)
    : m_context(context)
{
}

QString BorneoService::validate(const BorneoConfig& config) const
{
    if (config.eslora <= 0.0) {
        return QStringLiteral("Eslora debe ser mayor a 0");
    }

    if (config.grilletes < 0) {
        return QStringLiteral("Grilletes al agua debe ser mayor o igual a 0");
    }

    if (config.profundidad < 0.0) {
        return QStringLiteral("Profundidad debe ser mayor o igual a 0");
    }

    return QString();
}

BorneoOperationResult BorneoService::startSessionInternal(const BorneoConfig& config)
{
    const QString validationError = validate(config);
    if (!validationError.isEmpty()) {
        return { false, validationError };
    }

    // TODO (futuro): if (!isFondeoActive()) { return { false, "No hay punto de Fondeo activo" }; }

    const double radio = BorneoCalculator::calculateRadius(config);

    if (radio <= 0.0) {
        return { false, QStringLiteral("Advertencia: los valores ingresados producen un radio de Borneo invalido. Verifique eslora, grilletes y profundidad.").arg(radio, 0, 'f', 2) };
    }

    BorneoSessionState& session = m_context->borneoSession;
    session.config = config;
    session.radioCalculado = BorneoCalculator::calculateRadius(config);
    session.active = true;

    return { true, QStringLiteral("Sesion de Borneo iniciada. Radio: %1 mts").arg(session.radioCalculado, 0, 'f', 2) };
}

BorneoOperationResult BorneoService::startSession(const BorneoConfig& config)
{
    return startSessionInternal(config);
}

BorneoOperationResult BorneoService::startSessionByClase(const QString& clase, int grilletes, double profundidad)
{
    double esloraResuelta = 0.0;
    if (!BuqueClaseCatalog::resolveEslora(clase, esloraResuelta)) {
        return { false, QStringLiteral("Clase de buque no reconocida: %1").arg(clase) };
    }

    BorneoConfig config;
    config.eslora = esloraResuelta;
    config.grilletes = grilletes;
    config.profundidad = profundidad;

    return startSessionInternal(config);
}

BorneoOperationResult BorneoService::stopSession()
{
    BorneoSessionState& session = m_context->borneoSession;

    if (!session.active) {
        return { false, QStringLiteral("No hay sesion de Borneo activa") };
    }

    session.active = false;
    qDebug() << "[BorneoService] Sesion de Borneo finalizada";
    return { true, QStringLiteral("Sesion de Borneo finalizada") };
}