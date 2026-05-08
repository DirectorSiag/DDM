#include "obmservice.h"

#include "iOBMHandler.h"

#include <QtGlobal>

ObmService::ObmService(IOBMHandler* obmHandler)
    : m_obmHandler(obmHandler)
{
    Q_ASSERT(m_obmHandler);
}

QPair<double, double> ObmService::getCurrentPosition() const
{
    const auto position = m_obmHandler->getPosition();
    return {position.first, position.second};
}