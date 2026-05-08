#include "track.h"

#include "RadarMath.h"

#include <QtMath>
#include <cmath>

QString Track::formatHoursToHHMM(double hours)
{
    if (!std::isfinite(hours) || hours < 0.0) {
        return QStringLiteral("--:--");
    }

    const qint64 totalMinutes = static_cast<qint64>(std::round(hours * 60.0));
    const qint64 hh = totalMinutes / 60;
    const qint64 mm = totalMinutes % 60;
    return QStringLiteral("%1:%2")
        .arg(hh, 2, 10, QLatin1Char('0'))
        .arg(mm, 2, 10, QLatin1Char('0'));
}

// ---------- helpers ----------
double Track::normalize360(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

int Track::normalizeCourseInt(int deg)
{
    // deja en [0,359]
    deg %= 360;
    if (deg < 0) deg += 360;
    // En requerimiento el rango es 000..359
    if (deg == 360) deg = 0;
    return deg;
}

double Track::clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

QChar Track::identityToCode(Identity i)
{
    // Tabla SURFACE TRACKS:
    // P: Pendiente
    // A: Posible amigo
    // F: Confirmado amigo
    // E: Posible hostil
    // H: Confirmado hostil
    // U: Evaluado desconocido
    // Y: Helicóptero amigo
    switch (i) {
    case Identity::Pending:      return QLatin1Char('P');
    case Identity::PossFriend:   return QLatin1Char('A');
    case Identity::ConfFriend:   return QLatin1Char('F');
    case Identity::PossHostile:  return QLatin1Char('E');
    case Identity::ConfHostile:  return QLatin1Char('H');
    case Identity::EvalUnknown:  return QLatin1Char('U');
    case Identity::Heli:         return QLatin1Char('Y');
    default:                     return QLatin1Char('P');
    }
}

// ---------- ctor ----------
Track::Track(int id,
             Type type,
             Identity identity,
             TrackMode mode,
             float xDm,
             float yDm,
             double speedKnots,
             double courseDeg,
             Environment creationEnvironment)
    : m_id(id)
    , m_type(static_cast<uint8_t>(type))
    , m_creationEnvironment(static_cast<uint8_t>(creationEnvironment))
    , m_identity(static_cast<uint8_t>(identity))
    , m_mode(static_cast<uint8_t>(mode))
    , m_xDm(xDm)
    , m_yDm(yDm)
{
    // Curso
    m_courseDeg = normalize360(courseDeg);

    // Velocidad
    // Guardamos DM/h (requerimiento). Si nos pasan kt, convertimos.
    // kt = NM/h  -> DM/h = kt / 0.98747
    if (std::isnan(speedKnots) || speedKnots < 0.0) {
        m_speedDmPerHour = 0.0;
    } else {
        m_speedDmPerHour = clamp(speedKnots / kDmToNm, 0.0, kMaxVelDmPerHour);
    }

    // Defaults de estados Link según tabla
    // Link Y: "R si es un contacto recibido - cualquier otro caso".
    // Acá por default dejamos inválido; que el decoder/handler lo setee.
    m_estadoLinkY = static_cast<uint8_t>(LinkY_Invalid);
    m_estadoLink14 = static_cast<uint8_t>(Link14_Invalid);
}

// ---------- getters (compatibilidad) ----------
int Track::getId() const { return m_id; }
Track::Type Track::getType() const { return static_cast<Type>(m_type); }
Track::Environment Track::getCreationEnvironment() const { return static_cast<Environment>(m_creationEnvironment); }
Track::Identity Track::getIdentity() const { return static_cast<Identity>(m_identity); }
Track::TrackMode Track::getTrackMode() const { return static_cast<TrackMode>(m_mode); }

float Track::getX() const { return m_xDm; }
float Track::getY() const { return m_yDm; }

double Track::getSpeedKnots() const
{
    // DM/h -> kt
    return m_speedDmPerHour * kDmToNm;
}

double Track::getCourseDeg() const
{
    return m_courseDeg;
}

// ---------- requerimiento ----------
char Track::getTipo() const
{
    // Para esta herramienta: Surface Tracks
    return kDefaultTipo;
}

int Track::getNumero() const
{
    return m_id;
}

QChar Track::getIdentityCode() const
{
    return identityToCode(getIdentity());
}

QString Track::getInformacionAmpliatoria() const
{
    return m_infoAmpliatoria;
}

int Track::getCursoInt() const
{
    return static_cast<int>(normalize360(m_courseDeg));
}

double Track::getVelocidadDmPerHour() const
{
    return m_speedDmPerHour;
}

int Track::getAsignacionFc() const
{
    return static_cast<int>(m_asignacionFc);
}

QString Track::getCodigoAsignacion() const
{
    return m_codigoAsignacion;
}

Track::LinkYStatus Track::getEstadoLinkY() const
{
    return static_cast<LinkYStatus>(m_estadoLinkY);
}

Track::Link14Status Track::getEstadoLink14() const
{
    return static_cast<Link14Status>(m_estadoLink14);
}

QString Track::getCodigoPrivado() const
{
    return m_codigoPrivado;
}

Track::SitrepPppData Track::getSitrepPpp() const
{
    return m_sitrepPpp;
}

QString Track::getSitrepPppTimeHHMM() const
{
    if (m_sitrepPpp.status == SitrepPppData::NotComputed
        || m_sitrepPpp.status == SitrepPppData::NoOwnShip) {
        return QStringLiteral("--:--");
    }

    return formatHoursToHHMM(m_sitrepPpp.timeHours);
}

// ---------- setters (compatibilidad) ----------
void Track::setId(int id)
{
    // NÚMERO: 0001..7776 (si te viene fuera de rango, lo dejamos igual para no romper flujos)
    m_id = id;
}

void Track::setType(Type t) { m_type = static_cast<uint8_t>(t); }
void Track::setCreationEnvironment(Environment env) { m_creationEnvironment = static_cast<uint8_t>(env); }
void Track::setIdentity(Identity i) { m_identity = static_cast<uint8_t>(i); }
void Track::setTrackMode(TrackMode m) { m_mode = static_cast<uint8_t>(m); }

void Track::setX(float xDm) { m_xDm = xDm; }
void Track::setY(float yDm) { m_yDm = yDm; }

void Track::setSpeedKnots(double kt)
{
    if (std::isnan(kt) || kt < 0.0) {
        m_speedDmPerHour = 0.0;
        return;
    }
    m_speedDmPerHour = clamp(kt / kDmToNm, 0.0, kMaxVelDmPerHour);
}

void Track::setCourseDeg(double deg)
{
    m_courseDeg = normalize360(deg);
}

// ---------- setters (requerimiento) ----------
void Track::setInformacionAmpliatoria(const QString& text)
{
    m_infoAmpliatoria = text.isEmpty() ? QStringLiteral("-") : text;
}

void Track::setCursoInt(int deg)
{
    m_courseDeg = static_cast<double>(normalizeCourseInt(deg));
}

void Track::setVelocidadDmPerHour(double dmPerHour)
{
    if (std::isnan(dmPerHour)) {
        m_speedDmPerHour = 0.0;
        return;
    }
    m_speedDmPerHour = clamp(dmPerHour, 0.0, kMaxVelDmPerHour);
}

void Track::setAsignacionFc(int fc)
{
    if (fc >= 1 && fc <= 6) {
        m_asignacionFc = static_cast<uint8_t>(fc);
    } else {
        m_asignacionFc = 0; // inválido
    }
}

void Track::setCodigoAsignacion(const QString& code)
{
    m_codigoAsignacion = code.isEmpty() ? QStringLiteral("-") : code;
}

void Track::setEstadoLinkY(LinkYStatus st)
{
    m_estadoLinkY = static_cast<uint8_t>(st);
}

void Track::setEstadoLink14(Link14Status st)
{
    m_estadoLink14 = static_cast<uint8_t>(st);
}

void Track::setCodigoPrivado(const QString& code)
{
    m_codigoPrivado = code.isEmpty() ? QStringLiteral("-") : code;
}

void Track::setSitrepPpp(const SitrepPppData& ppp)
{
    m_sitrepPpp = ppp;
}

// ---------- cálculos ----------
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
    if (deltaTimeSeconds <= 0.0)
        return;

    if (m_speedDmPerHour <= 0.0)
        return;

    const double distanceDm = m_speedDmPerHour * (deltaTimeSeconds / 3600.0);
    const double courseRad  = qDegreesToRadians(m_courseDeg);

    // Convención: +y = Norte, +x = Este
    const double dx = distanceDm * std::sin(courseRad);
    const double dy = distanceDm * std::cos(courseRad);

    m_xDm = static_cast<float>(m_xDm + dx);
    m_yDm = static_cast<float>(m_yDm + dy);
}

QString Track::toString() const
{
    // Mostramos velocidad en kt (por compatibilidad con logs / SITREP)
    return QStringLiteral(
             "Track{tipo=%1, env=%2, numero=%3, ident=%4, pos=(%5,%6)DM, az=%7°, dt=%8DM, spd=%9kt (%10DM/h), crs=%11°, FC=%12, ASG=%13, LY=%14, L14=%15, info=%16, priv=%17}")
        .arg(QLatin1Char(getTipo()))
         .arg(TrackData::toQString(getCreationEnvironment()))
        .arg(m_id)
        .arg(getIdentityCode())
        .arg(double(m_xDm), 0, 'f', 2)
        .arg(double(m_yDm), 0, 'f', 2)
        .arg(getAzimuthDeg(), 0, 'f', 1)
        .arg(getDistanceDm(), 0, 'f', 2)
        .arg(getSpeedKnots(), 0, 'f', 1)
        .arg(getVelocidadDmPerHour(), 0, 'f', 1)
        .arg(m_courseDeg, 0, 'f', 1)
        .arg(getAsignacionFc())
        .arg(getCodigoAsignacion())
        .arg(int(getEstadoLinkY()))
        .arg(int(getEstadoLink14()))
        .arg(getInformacionAmpliatoria())
        .arg(getCodigoPrivado());
}

double Track::speedKnots() const
{
    return m_speedDmPerHour;
}

double Track::course() const
{
    return m_courseDeg;
}

void Track::setCourse(double newCourse)
{
    m_courseDeg = newCourse;
}
