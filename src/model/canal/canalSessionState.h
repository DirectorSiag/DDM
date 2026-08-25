#ifndef CANAL_SESSION_STATE_H
#define CANAL_SESSION_STATE_H

#include <QString>

// Estructura de entrada (Lo que manda el Command al Service)
struct CanalConfig {
    bool setA = false; int trackA = -1;
    bool setB = false; int trackB = -1;
    bool setC = false; int trackC = -1;
    bool setD = false; int trackD = -1;
};

// Representa una sola columna en la UI
struct CanalSlot {
    bool active = false;
    int trackId = -1;

    // Outputs cinemáticos
    double azimutVerdadero = 0.0;
    double distanciaYardas = 0.0;
    double rumboVerdadero = 0.0;
    double timeToArrivalMin = 0.0;
    bool etaValid = false;

    // Outputs de Interfaz/Alarma
    bool isAlarmActive = false;
    double marcacionRelativa = 0.0;
    bool hasTriggeredAlarm = false; // Recuerda si la boya ya hizo sonar la alarma

    void reset() { *this = CanalSlot{}; }
};

struct CanalSessionState {
    bool active = false; // Flag global de la sesión (Botón INICIAR)
    CanalSlot columnas[4]; // 0=A, 1=B, 2=C, 3=D

    void reset() { *this = CanalSessionState{}; }
};

#endif // CANAL_SESSION_STATE_H