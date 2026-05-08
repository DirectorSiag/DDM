#pragma once

#include <cstdint>

#include "enums/enums.h"   // TrackData::{Type,Identity,TrackMode}
#include <QString>

// Modelo de datos de Track (Surface Tracks)
// Requerimientos: ver tabla "SURFACE TRACKS" (TIPO, NÚMERO, IDENTIDAD, INFO, CURSO, VELOCIDAD, etc.)
class Track
{
public:
    using Type      = TrackData::Type;
    using Identity  = TrackData::Identity;
    using TrackMode = TrackData::TrackMode;
    using Environment = TrackData::Type;

    // Estados según requerimiento
    enum LinkYStatus : uint8_t {
        LinkY_ReceivedOnly = 0,   // R
        LinkY_Correlated   = 1,   // C
        LinkY_Tx           = 2,   // T
        LinkY_TxS          = 3,   // S
        LinkY_Invalid      = 255
    };

    enum Link14Status : uint8_t {
        Link14_Tx      = 0,       // T transmitido
        Link14_Invalid = 255
    };

    // PPP para SITREP: calculado entre este Track y OwnShip.
    struct SitrepPppData {
        enum Status : uint8_t {
            NotComputed = 0,
            NoOwnShip,
            DegenerateRelativeMotion,
            Valid
        };

        double azDeg = 0.0;
        double distanceDm = 0.0;
        double timeHours = 0.0;
        Status status = NotComputed;
        QString reason;
    };

    static constexpr char   kDefaultTipo = 'S';
    static constexpr int    kMinNumero = 1;
    static constexpr int    kMaxNumero = 7776;
    static constexpr int    kMinCurso  = 0;
    static constexpr int    kMaxCurso  = 359;
    static constexpr double kMaxVelDmPerHour = 99.9;

    // 1 DM = 0.98747 NM  =>  DM/h * 0.98747 = kt
    static constexpr double kDmToNm = 0.98747;

    Track() = default;

    // Constructor principal (compatibilidad con el código existente)
    // - id se interpreta como NÚMERO de track (0001..7776)
    // - speedKnots/courseDeg se conservan, pero internamente se guarda velocidad en DM/h
    Track(int id,
          Type type,
          Identity identity,
          TrackMode mode,
          float xDm,
          float yDm,
          double speedKnots = 0.0,
            double courseDeg  = 0.0,
            Environment creationEnvironment = TrackData::SPC);

    // --- Getters principales (compatibilidad) ---
    int getId() const;
    Type getType() const;
    Environment getCreationEnvironment() const;
    Identity getIdentity() const;
    TrackMode getTrackMode() const;

    float  getX() const;              // DM
    float  getY() const;              // DM

    // Nota: el requerimiento define VELOCIDAD en DM/h.
    // Estas funciones mantienen la API actual.
    double getSpeedKnots() const;     // kt (derivado)
    double getCourseDeg() const;      // deg [0..360)

    double getAzimuthDeg() const;     // deg [0..360)
    double getDistanceDm() const;     // DM

    // --- Requerimiento "SURFACE TRACKS" ---
    char    getTipo() const;                  // 'S'
    int     getNumero() const;                // alias de id

    // Identidad como código de 1 letra según tabla (P/A/F/E/H/U/Y)
    QChar   getIdentityCode() const;

    QString getInformacionAmpliatoria() const;

    int     getCursoInt() const;              // 0..359 (entero)

    double  getVelocidadDmPerHour() const;    // 0..99.9 DM/h

    int     getAsignacionFc() const;          // 1..6, 0 = inválido
    QString getCodigoAsignacion() const;      // alfanumérico, "-" si vacío

    LinkYStatus  getEstadoLinkY() const;
    Link14Status getEstadoLink14() const;

    QString getCodigoPrivado() const;

    SitrepPppData getSitrepPpp() const;
    QString getSitrepPppTimeHHMM() const;

    // --- Setters ---
    void setId(int id);
    void setType(Type t);
    void setCreationEnvironment(Environment env);
    void setIdentity(Identity i);
    void setTrackMode(TrackMode m);

    void setX(float xDm);
    void setY(float yDm);

    // API actual: set en knots (internamente se convierte a DM/h)
    void setSpeedKnots(double kt);
    void setCourseDeg(double deg);

    // Nuevos setters (requerimiento)
    void setInformacionAmpliatoria(const QString& text);

    // Curso entero 0..359 (normaliza)
    void setCursoInt(int deg);

    // Velocidad en DM/h (clampa al rango 0..99.9)
    void setVelocidadDmPerHour(double dmPerHour);

    // 1..6, 0 => inválido
    void setAsignacionFc(int fc);

    void setCodigoAsignacion(const QString& code);

    void setEstadoLinkY(LinkYStatus st);
    void setEstadoLink14(Link14Status st);

    void setCodigoPrivado(const QString& code);

    void setSitrepPpp(const SitrepPppData& ppp);

    // deltaTimeSeconds: segundos transcurridos
    void updatePosition(double deltaTimeSeconds);

    QString toString() const;

    double speedKnots() const;

    double course() const;
    void setCourse(double newCourse);

private:
    static double normalize360(double deg);
    static int normalizeCourseInt(int deg);
    static double clamp(double v, double lo, double hi);

    static QChar identityToCode(Identity i);
    static QString formatHoursToHHMM(double hours);

private:
    // Campos base
    int32_t  m_id{0};
    uint8_t  m_type{0};
    uint8_t  m_creationEnvironment{0};
    uint8_t  m_identity{0};
    uint8_t  m_mode{0};

    float    m_xDm{0.0f};
    float    m_yDm{0.0f};

    // Requerimiento: velocidad en DM/h y curso en grados
    double   m_speedDmPerHour{0.0};
    double   m_courseDeg{0.0};

    // Campos extra requeridos
    QString  m_infoAmpliatoria{QStringLiteral("-")};

    // Asignación FC (1..6), 0 inválido
    uint8_t  m_asignacionFc{0};

    QString  m_codigoAsignacion{QStringLiteral("-")};

    uint8_t  m_estadoLinkY{static_cast<uint8_t>(LinkY_Invalid)};
    uint8_t  m_estadoLink14{static_cast<uint8_t>(Link14_Invalid)};

    QString  m_codigoPrivado{QStringLiteral("-")};

    SitrepPppData m_sitrepPpp;
};
