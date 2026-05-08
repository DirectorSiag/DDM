#pragma once

#include <QPair>

class IOBMHandler;

class ObmService {
public:
    explicit ObmService(IOBMHandler* obmHandler);

    QPair<double, double> getCurrentPosition() const;

private:
    IOBMHandler* m_obmHandler;
};