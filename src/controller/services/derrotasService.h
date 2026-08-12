#pragma once
#include "commandContext.h"
#include "model/derrotas/derrotasLogManager.h"

struct DerrotasOperationResult {
    bool success = false;
    QString message;
};

class DerrotasService {
public:
    explicit DerrotasService(CommandContext* ctx);

    // Derrotas Futuras
    DerrotasOperationResult startFutura(int trackId, int timeMinutes, double thresholdDeg, double thresholdKn);
    DerrotasOperationResult stopFutura();

    // Derrotas Pasadas
    DerrotasOperationResult startRecording(int trackId);
    DerrotasOperationResult stopRecording();

    // Botón BORRAR: corta futura y grabación activas.
    DerrotasOperationResult clearAll();

    void update(); // llamado cada 80ms desde main.cpp

private:
    bool isValidTime(int minutes) const;
    bool isValidThresholdDeg(double deg) const;
    bool isValidThresholdKn(double kn) const;

    CommandContext*    m_ctx;
    DerrotasLogManager m_logManager;
};