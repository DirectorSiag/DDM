#ifndef DELETECIRCLECOMMAND_H
#define DELETECIRCLECOMMAND_H

#include "iCommand.h"

class DeleteCircleCommand : public ICommand {
public:
    // Este comando elimina circulos desde CLI; en AR-TDC se corresponde con delete_circle.
    QString getName() const override { return "deleteCircle"; }
    QString getDescription() const override { return "Elimina un círculo y sus cursores asociados dado su ID."; }
    QString usage() const override { return "deleteCircle(id)"; }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // DELETECIRCLECOMMAND_H
