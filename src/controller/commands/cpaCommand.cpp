#include "cpaCommand.h"
#include "../services/cpaservice.h"

CommandResult CpaCommand::execute(const CommandInvocation &inv, CommandContext &ctx) const
{
    const QStringList& args = inv.args;

    if (args.size() < 2) {
        usage();
    }

    bool ok1 = false, ok2 = false;
    int idTrack1 = args[0].toInt(&ok1);
    int idTrack2 = args[1].toInt(&ok2);

    if (!ok1 || !ok2) {
        return {false, "IDs invalidos"};
    }

    CPAService cpaService(&ctx);
    //CPAResult ret = cpaService.computeCPA(idTrack1, idTrack2);

    //if (!ret.valid) {
    //    return {false, "Contactos no encontrados"};
    //}

    QString out;
    QTextStream oss(&out);

    oss << "CPA RESULT\n";
    oss << "Tracks: " << idTrack1 << " vs " << idTrack2 << "\n";
    //oss << "TCPA: " << ret.tcpa << " s\n";
    //oss << "CPA Distance: " << ret.dcpa << " DM\n";

    //if (ret.tcpa < 0) {
    //    oss << "Estado: Divergiendo\n";
    //} else {
    //    oss << "Estado: Convergente\n";
    //}

    return {true, out};

}
