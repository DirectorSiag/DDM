#pragma once

#include <QString>

class EstacionamientoCalculator
{
public:
    struct KinematicState {
        double xDm = 0.0;
        double yDm = 0.0;
        double speedDmPerHour = 0.0;
        double courseDeg = 0.0;
        bool valid = true;
    };

    struct Input {
        KinematicState trackA;
        KinematicState trackB;
        double azRelativeDeg = 0.0;
        double distanceDm = 0.0;

        bool useSpeedMode = true;
        double vdDmPerHour = 0.0;
        double duHours = 0.0;
    };

    struct Result {
        enum Status {
            Valid = 0,
            InvalidInput,
            NoPositiveTimeSolution,
            DegenerateSolution
        };

        Status status = InvalidInput;
        QString reason;
        double rumboDeg = 0.0;
        double timeHours = 0.0;
        double resultingSpeedDmPerHour = 0.0;
    };

    static Result compute(const Input& input);
};
