#include "track.h"
#include "RadarMath.h"
#include <QtMath>      // qDegreesToRadians
#include <cmath>

static double normalize360(double deg)
{
    // deja en [0, 360)
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

Track::Track(int id, Type type, Identity identity, TrackMode mode, float xDm, float yDm, double speedKnots, double courseDeg)
    : m_id(id)
    , m_type(static_cast<uint8_t>(type))
    , m_identity(static_cast<uint8_t>(identity))
    , m_mode(static_cast<uint8_t>(mode))
    , m_xDm(xDm)
    , m_yDm(yDm)
    , m_speedKnots(speedKnots)
    , m_courseDeg(normalize360(courseDeg))
{
}

int Track::getId() const { return m_id; }
Track::Type Track::getType() const { return static_cast<Type>(m_type); }
Track::Identity Track::getIdentity() const { return static_cast<Identity>(m_identity); }
Track::TrackMode Track::getTrackMode() const { return static_cast<TrackMode>(m_mode); }

float Track::getX() const { return m_xDm; }
float Track::getY() const { return m_yDm; }

double   m_speedKnots{std::numeric_limits<double>::quiet_NaN()};
double   m_courseDeg{std::numeric_limits<double>::quiet_NaN()};

void Track::setId(int id) { m_id = id; }
void Track::setType(Type t) { m_type = static_cast<uint8_t>(t); }
void Track::setIdentity(Identity i) { m_identity = static_cast<uint8_t>(i); }
void Track::setTrackMode(TrackMode m) { m_mode = static_cast<uint8_t>(m); }

void Track::setX(float xDm) { m_xDm = xDm; }
void Track::setY(float yDm) { m_yDm = yDm; }
void Track::setSpeedKnots(double kt) { m_speedKnots = kt; }
void Track::setCourseDeg(double deg) { m_courseDeg = normalize360(deg); }

double Track::getSpeedKnots() const { return m_speedKnots; }
double Track::getCourseDeg() const  { return m_courseDeg; }   // o m_courseDeg si lo renombraste

double Track::getAzimuthDeg() const
{
    return RadarMath::azimuthDeg(static_cast<double>(m_xDm),
                                 static_cast<double>(m_yDm));
}

double Track::getDistanceDm() const
{
    return RadarMath::distanceDm(static_cast<double>(m_xDm),
                                 static_cast<double>(m_yDm));
}

void Track::updatePosition(double deltaTimeSeconds)
{
    if (std::isnan(m_speedKnots) || std::isnan(m_courseDeg))
        return;

    if (m_speedKnots <= 0.0 || deltaTimeSeconds <= 0.0)
        return;

    const double distanceDm = m_speedKnots * (deltaTimeSeconds / 3600.0);
    const double courseRad = qDegreesToRadians(m_courseDeg);


    const double dx = distanceDm * std::sin(courseRad);
    const double dy = distanceDm * std::cos(courseRad);

    m_xDm = static_cast<float>(m_xDm + dx);
    m_yDm = static_cast<float>(m_yDm + dy);
}

QString Track::toString() const
{
    return QStringLiteral("Track{id=%1, type=%2, identity=%3, mode=%4, pos=(%5, %6), spd=%7kt, crs=%8°}")
        .arg(m_id)
        .arg(TrackData::toQString(getType()))
        .arg(TrackData::toQString(getIdentity()))
        .arg(TrackData::toQString(getTrackMode()))
        .arg(double(m_xDm), 0, 'f', 2)
        .arg(double(m_yDm), 0, 'f', 2)
        .arg(m_speedKnots, 0, 'f', 1)
        .arg(m_courseDeg,  0, 'f', 1);
}
