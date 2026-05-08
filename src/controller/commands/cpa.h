#ifndef CPA_H
#define CPA_H

#include <QObject>
#include "src/model/entities/track.h"
#include <QPair>

struct CPAResult {
    double tcpa;    // Tiempo en segundos
    double dcpa;    // Distancia en DM
    bool valid;
};

class CPA : public QObject
{
    Q_OBJECT
public:
    explicit CPA(QObject *parent = nullptr);
    CPAResult computeCPA(const Track& track_a, const Track& track_b);

signals:
};

#endif // 
