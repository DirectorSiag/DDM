#pragma once

#include "commandContext.h"

class FondeoService {
public:
    explicit FondeoService(CommandContext* ctx);

    bool startSession(const FondeoConfig& config, QString& outError);
    void stopSession();
    void update();

private:
    CommandContext* m_ctx;
};