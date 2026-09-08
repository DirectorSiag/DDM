#ifndef CANAL_CALCULATOR_H
#define CANAL_CALCULATOR_H

#include <QPointF>
#include "canalSessionState.h"

class CanalCalculator {
public:
    static void calculateCinematica(const QPointF& ownPos, double ownCourse, double ownSpeedDm,
                                    const QPointF& buoyPos, CanalSlot& outSlot);

    static bool isWithinAlarmWindow(double relativeBearing);
    static bool isBehind(double relativeBearing);
};

#endif // CANAL_CALCULATOR_H