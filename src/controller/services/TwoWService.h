#pragma once

#include "commandContext.h"

class TwoWService {
public:
    explicit TwoWService(CommandContext* ctx);

    void startSession(int guideTrackId, int bpStation, double circleRadiusNm);
    void stopSession();
    void update();

private:
    CommandContext* m_ctx;
};