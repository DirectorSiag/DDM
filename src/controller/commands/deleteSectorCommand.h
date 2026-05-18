#ifndef DELETESECTORCOMMAND_H
#define DELETESECTORCOMMAND_H

#include "iCommand.h"

class DeleteSectorCommand : public ICommand {
    Q_OBJECT
public:
    QString getName()        const override { return "deleteSector"; }
    QString getDescription() const override { return "Elimina un sector y sus cursores asociados dado su ID."; }
    QString usage()          const override { return "deleteSector(id)"; }

    CommandResult execute(const CommandInvocation& inv, CommandContext& ctx) const override;
};

#endif // DELETESECTORCOMMAND_H
