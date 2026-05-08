#include "cpa.h"

CPA::CPA(QObject *parent)
    : QObject{parent}
{}

CPAResult CPA::computeCPA(const Track &track_a, const Track &track_b)
{

    QPair<double,double> Pa = {track_a.getX(), track_a.getY()};
    double Va = track_a.speedKnots(); // DM/s

    QPair<double,double> Pb = {track_b.getX(), track_b.getY()};
    double Vb = track_b.speedKnots();

    QPair<double,double> Pr {
        Pb.first - Pa.first,
        Pb.second - Pa.second
    };

    QPair<double,double> Vr {
        Vb - Va,
        Vb - Va
    };

    double vr2 = std::sqrt(Vr.first) + std::sqrt(Vr.second);
    if (vr2 < 1e-9)
        return {0, 0, false};

    double tcpa = - ((Pr.first * Vr.first) + (Pr.second * Vr.second)) / vr2;

    // opcional: evitar pasado
    if (tcpa < 0)
        tcpa = 0;

    QPair<double,double> Pcpa {
        Pr.first + Vr.first * tcpa,
        Pr.second + Vr.second * tcpa
    };

    double dcpa = std::sqrt(std::sqrt(Pcpa.first) + std::sqrt(Pcpa.second));

    return { tcpa, dcpa, true };
}
