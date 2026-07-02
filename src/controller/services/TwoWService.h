#pragma once
#include "commandContext.h"

struct TwoWOperationResult {
    bool    ok = false;
    QString message;
};

class TwoWService {
public:
    explicit TwoWService(CommandContext* ctx);
    TwoWOperationResult startSession(int guideTrackId, int bpStation, double circleRadiusNm, const QList<int>& aliadas = {});
    TwoWOperationResult stopSession();
    void update();
private:
    CommandContext* m_ctx;
};