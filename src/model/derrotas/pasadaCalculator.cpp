#include "pasadaCalculator.h"
#include "RadarMath.h"

void PasadaCalculator::buildTrackPoints(
    const QList<DerrotasLogPoint>& logPoints,
    double originLatDeg,
    double originLonDeg,
    QList<QPointF>& out_trackPoints)
{
    out_trackPoints.clear();
    for (const DerrotasLogPoint& p : logPoints) {
        double xDm = 0.0;
        double yDm = 0.0;
        RadarMath::latLonToDm(originLatDeg, originLonDeg, p.latDeg, p.lonDeg, xDm, yDm);
        out_trackPoints.append(QPointF(xDm, yDm));
    }
}

void PasadaCalculator::buildWayPoints(
    const QList<DerrotasLogPoint>& logPoints,
    double originLatDeg,
    double originLonDeg,
    QList<DerrotasWayPoint>& out_wayPoints)
{
    out_wayPoints.clear();
    if (logPoints.isEmpty())
        return;

    for (int i = 0; i < logPoints.size(); ++i) {
        const DerrotasLogPoint& p = logPoints[i];

        const bool isFirst = (i == 0);
        const bool isLast  = (i == logPoints.size() - 1);

        // cualquier punto intermedio cuyo RV difiera del anterior genera un Way Point.
        const bool courseChanged = !isFirst && (p.rvDeg != logPoints[i - 1].rvDeg);

        if (!isFirst && !isLast && !courseChanged)
            continue;

        double xDm = 0.0;
        double yDm = 0.0;
        RadarMath::latLonToDm(originLatDeg, originLonDeg, p.latDeg, p.lonDeg, xDm, yDm);

        DerrotasWayPoint wp;
        wp.position   = QPointF(xDm, yDm);
        wp.isEndpoint = isFirst || isLast;
        wp.rvDeg      = p.rvDeg;
        wp.vdKn       = p.vdKn;

        //fecha/hora solo en los extremos.
        if (wp.isEndpoint) {
            wp.dateTimeLabel = p.timestamp.toString(QStringLiteral("ddMMyy HH:mm:ss"));
        }

        out_wayPoints.append(wp);
    }
}