#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "buqueClaseCatalog.h"

static constexpr double kEps = 0.001;

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/buqueClaseCatalog_cases.json");

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll())
        .object()["buqueClaseCatalogTestCases"]
        .toArray();
}

class TestBuqueClaseCatalog : public QObject
{
    Q_OBJECT

    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const
    {
        for (const QJsonValue& value : m_cases) {
            const QJsonObject object = value.toObject();

            if (object["id"].toString() == id) {
                return object;
            }
        }

        return {};
    }

private slots:

    void initTestCase()
    {
        m_cases = loadCases();

        QVERIFY2(
            !m_cases.isEmpty(),
            "JSON de casos no cargado. Revisa el resources_test.qrc"
        );
    }

    // ── resolveEslora ─────────────────────────────────────────────

    void test_CAT_01_Resolve_Meko360()
    {
        runResolveTest("CAT_01_Resolve_Meko360");
    }

    void test_CAT_02_Resolve_Meko140()
    {
        runResolveTest("CAT_02_Resolve_Meko140");
    }

    void test_CAT_03_Resolve_Patagonia()
    {
        runResolveTest("CAT_03_Resolve_Patagonia");
    }

    void test_CAT_04_Resolve_Minusculas()
    {
        runResolveTest("CAT_04_Resolve_Minusculas");
    }

    void test_CAT_05_Resolve_EspaciosLaterales()
    {
        runResolveTest("CAT_05_Resolve_EspaciosLaterales");
    }

    void test_CAT_06_ClaseInexistente()
    {
        runResolveTest("CAT_06_ClaseInexistente");
    }

    void test_CAT_07_ClaseVacia()
    {
        runResolveTest("CAT_07_ClaseVacia");
    }

    void test_CAT_08_MinusculasConEspacios()
    {
        runResolveTest("CAT_08_MinusculasConEspacios");
    }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para resolveEslora
    // ─────────────────────────────────────────────────────────────
    void runResolveTest(const QString& id)
    {
        const QJsonObject tc = findCase(id);

        QVERIFY2(
            !tc.isEmpty(),
            qPrintable(QString("No se encontro el caso JSON: %1").arg(id))
        );

        const QJsonObject in = tc["inputs"].toObject();
        const QJsonObject ex = tc["expected"].toObject();

        // ARRANGE
        const QString clase = in["clase"].toString();
        double eslora = in["initialOutEslora"].toDouble();

        // ACT
        const bool result =
            BuqueClaseCatalog::resolveEslora(clase, eslora);

        // ASSERT
        QCOMPARE(result, ex["success"].toBool());

        const double expectedEslora = ex["eslora"].toDouble();

        QVERIFY2(
            qAbs(eslora - expectedEslora) < kEps,
            qPrintable(
                QString("[%1] Eslora esperada: %2 | Eslora obtenida: %3")
                    .arg(id)
                    .arg(expectedEslora, 0, 'f', 3)
                    .arg(eslora, 0, 'f', 3)
            )
        );
    }
};

QTEST_APPLESS_MAIN(TestBuqueClaseCatalog)

#include "test_buqueClaseCatalog.moc"