#pragma once

#include "commandContext.h"
#include "model/canal/canalSessionState.h"

struct CanalOperationResult {
    bool success;
    QString message;
};

class CanalService {
public:
    explicit CanalService(CommandContext* ctx);

    CanalOperationResult startSession(const CanalConfig& config);

    CanalOperationResult stopSession();

    void update();

private:
    CommandContext* m_ctx;
};