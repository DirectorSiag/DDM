#pragma once

#include <QPointF>
#include "haSessionState.h"

class HaCalculator {
public:

    static void calculate(
        QPointF         ownPos,
        double          ownCourseDeg,
        double          ownSpeedDm,
        QPointF         fallPoint,
        HaSessionState& outState
        );
private:
    static QString computeBanda(double angleFromProw);
};