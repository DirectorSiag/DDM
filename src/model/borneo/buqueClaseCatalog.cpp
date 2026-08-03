#include "buqueClaseCatalog.h"

namespace {
const QMap<QString, double>& catalog()
{
    static const QMap<QString, double> data = {
                                               { QStringLiteral("MEKO_360"), 125.0 },
                                               { QStringLiteral("MEKO_140"), 91.2 },
                                               { QStringLiteral("PATAGONIA"), 157.8 },
                                               };
    return data;
}
}

bool BuqueClaseCatalog::resolveEslora(const QString& clase, double& outEslora)
{
    const QString key = clase.trimmed().toUpper();

    const auto& data = catalog();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        if (it.key().toUpper() == key) {
            outEslora = it.value();
            return true;
        }
    }
    return false;
}

QList<QPair<QString, double>> BuqueClaseCatalog::allClases()
{
    QList<QPair<QString, double>> result;
    const auto& data = catalog();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        result.append({ it.key(), it.value() });
    }
    return result;
}