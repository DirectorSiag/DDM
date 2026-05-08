#pragma once

#include <QString>

// Motor matematico PPP/CPA reutilizable para cualquier par de entidades cinematicas.
// No conoce casos de uso (SITREP, GUI, comandos): solo calcula sobre estados.
class PppCalculator
{
public:
    struct KinematicState {
        double xDm = 0.0;
        double yDm = 0.0;
        double speedDmPerHour = 0.0;
        double courseDeg = 0.0;
        bool valid = true;
    };

    struct Result {
        enum Status {
            Valid = 0,
            DegenerateRelativeMotion,
            InvalidInput
        };

        double azDeg = 0.0;
        double distanceDm = 0.0;
        double timeHours = 0.0;
        Status status = InvalidInput;
        QString reason;
    };

    // reference y target definen el sistema relativo target-reference.
    static Result compute(const KinematicState& reference,
                          const KinematicState& target);
};
