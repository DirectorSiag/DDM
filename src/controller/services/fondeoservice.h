#pragma once

#include "commandContext.h"

struct FondeoOperationResult {
    bool success;
    QString message;
};

class FondeoService {
public:
    explicit FondeoService(CommandContext* ctx);

    FondeoOperationResult startSession(const FondeoConfig& config);
    FondeoOperationResult stopSession();
    void update();

private:
    void createFigures();
    void deleteFigures();

    CommandContext* m_ctx;
};