#pragma once
#include "iCommand.h"
#include "commandContext.h"

class SitrepCommand : public ICommand {
    Q_OBJECT
public:
    QString getName() const override { return "sitrep"; }
    QString getDescription() const override { return "SITREP: planilla de tracks. 'list' toma una foto estática; 'watch' la actualiza en tiempo real (bloquea la consola hasta Ctrl+C); 'delete' borra el track y también cancela sus sesiones de estacionamiento activas; 'info' asocia un texto al track."; }
    QString usage() const override {
        return "sitrep [list]     (foto estática, tabla de tracks)\n"
               "sitrep watch      (actualización continua, bloqueante)\n"
               "sitrep delete <trackId>   (borra track + sesiones de estacionamiento asociadas)\n"
               "sitrep info <trackId> <texto>\n"
               "Ejemplo: sitrep info 0012 contacto reevaluado";
    }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};
