#pragma once
#include "iCommand.h"
class DerrotasService;
class DerrotasCommand : public ICommand
{
    Q_OBJECT
public:
    explicit DerrotasCommand(DerrotasService* derrotasService) : m_derrotasService(derrotasService) {}
    QString getName() const override { return QStringLiteral("derrotas"); }
    QString getDescription() const override {
        return QStringLiteral("Gestiona las Derrotas (futuras y pasadas)");
    }
    QString usage() const override {
        return QStringLiteral(
            "Derrotas Futuras:\n"
            "  derrotas --track=<id> --tiempo=<5|10|30|60> --grados=<deg> --nudos=<kn>\n"
            "  derrotas --finalizar\n"
            "  derrotas --borrar\n"
            "  derrotas --info\n\n"
            "Derrotas Pasadas (grabacion):\n"
            "  derrotas --rec-iniciar --track=<id>\n"
            "  derrotas --rec-finalizar\n"
            );
    }
    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
private:
    DerrotasService* m_derrotasService;
};