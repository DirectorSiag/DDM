#pragma once

#include <QString>

class CommandContext;
struct BorneoConfig;

struct BorneoOperationResult {
    bool success;
    QString message;
};

class BorneoService
{
public:
    explicit BorneoService(CommandContext* context);

    BorneoOperationResult startSession(const BorneoConfig& config);

    // Modo catalogo: el operador selecciono una clase de buque (ej "MEKO 360"),
    // se resuelve la eslora via BuqueClaseCatalog antes de validar/calcular.
    BorneoOperationResult startSessionByClase(const QString& clase, int grilletes, double profundidad);

    BorneoOperationResult stopSession();

    // TODO (futuro, pendiente): método para verificar si hay un punto de Fondeo
    // activo antes de permitir startSession. No se llama todavía desde ningún lado.
    // bool isFondeoActive() const;

private:
    CommandContext* m_context;

    QString validate(const BorneoConfig& config) const;
    BorneoOperationResult startSessionInternal(const BorneoConfig& config);
};