#pragma once
#include "iCommand.h"

class ObmService;

class HaCommand : public ICommand
{
    Q_OBJECT
public:
    explicit HaCommand(ObmService* obmService) : m_obmService(obmService) {}

    QString getName() const override { return QStringLiteral("ha"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona emergencias Hombre al Agua: inicia sesión (por popa, cursor, lat/lon o azimut+distancia), detiene (--stop), consulta (--info) o lista (--list) todas las activas.");
    }
    QString usage() const override {
        return QStringLiteral(
            "ha --popa\n"
            "ha --cursor\n"
            "ha --latlon --lat=<deg>,<min>,<sec> --lon=<deg>,<min>,<sec>\n"
            "ha --az=<azimut> --d=<distancia_yardas>\n"
            "ha --stop [<slot>]   (sin slot: detiene todas las emergencias activas)\n"
            "ha --info <slot>\n"
            "ha --list             (lista todos los slots de emergencia activos)\n"
            "Ejemplo: ha --popa\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;

private:
    ObmService* m_obmService;
};
