#pragma once
#include <limits>

#include "enums/enums.h"   // o "enums.h", pero uno solo
#include <QString>
#include <cstdint>



class Track
{
public:
    using Type      = TrackData::Type;
    using Identity  = TrackData::Identity;
    using TrackMode = TrackData::TrackMode;

    Track() = default;

    Track(int id,
          Type type,
          Identity identity,
          TrackMode mode,
          float xDm,
          float yDm,
          double speedKnots = 0.0,
          double courseDeg  = 0.0);

    // --- Getters ---
    int getId() const;
    Type getType() const;
    Identity getIdentity() const;
    TrackMode getTrackMode() const;

    float  getX() const;              // DM
    float  getY() const;              // DM
    double getSpeedKnots() const;
    double getCourseDeg() const;

    double getAzimuthDeg() const;      // deg [0..360)
    double getDistanceDm() const;      // DM

    // --- Setters ---
    void setId(int id);
    void setType(Type t);
    void setIdentity(Identity i);
    void setTrackMode(TrackMode m);

    void setX(float xDm);
    void setY(float yDm);
    void setSpeedKnots(double kt);
    void setCourseDeg(double deg);

    // deltaTimeSeconds: segundos transcurridos
    void updatePosition(double deltaTimeSeconds);

    QString toString() const;

private:
    int32_t  m_id{0};
    uint8_t  m_type{0};
    uint8_t  m_identity{0};
    uint8_t  m_mode{0};

    float    m_xDm{0.0f};
    float    m_yDm{0.0f};

    double   m_speedKnots{std::numeric_limits<double>::quiet_NaN()};
    double   m_courseDeg{std::numeric_limits<double>::quiet_NaN()};

};
