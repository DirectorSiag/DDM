#include "centerservice.h"
#include "../../model/commandContext.h"

CenterService::CenterService(CommandContext* context)
    : m_context(context)
{
}

void CenterService::setCenter(double x, double y) {
    if (!m_context) return;
    m_context->setCenter({static_cast<float>(x), static_cast<float>(y)});
}

QPair<float,float> CenterService::getCenter() const {
    if (!m_context) return {0.0f, 0.0f};
    return { static_cast<float>(m_context->centerX), static_cast<float>(m_context->centerY) };
}
