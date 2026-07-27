#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "borneoCalculator.h"
#include "borneoSessionState.h"

static constexpr double kEps = 0.001;

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/borneocalculator_cases.json");

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll())
        .object()["borneoCalculatorTestCases"]
        .toArray();
}

class TestBorneoCalculator : public QObject
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

    // ── calculateRadius ──────────────────────────────────────────

    void test_CALC_01_Radio_Meko360_DosGrilletes()
    {
        runCalculateRadiusTest("CALC_01_Radio_Meko360_DosGrilletes");
    }

    void test_CALC_02_Radio_EsloraManual()
    {
        runCalculateRadiusTest("CALC_02_Radio_EsloraManual");
    }

    void test_CALC_03_Radio_ProfundidadDecimal()
    {
        runCalculateRadiusTest("CALC_03_Radio_ProfundidadDecimal");
    }

    void test_CALC_04_Radio_CeroGrilletes()
    {
        runCalculateRadiusTest("CALC_04_Radio_CeroGrilletes");
    }

    void test_CALC_05_Radio_ProfundidadCero()
    {
        runCalculateRadiusTest("CALC_05_Radio_ProfundidadCero");
    }

    void test_CALC_06_Radio_ExactamenteCero()
    {
        runCalculateRadiusTest("CALC_06_Radio_ExactamenteCero");
    }

    void test_CALC_07_Radio_Negativo()
    {
        runCalculateRadiusTest("CALC_07_Radio_Negativo");
    }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para calculateRadius
    // ─────────────────────────────────────────────────────────────
    void runCalculateRadiusTest(const QString& id)
    {
        const QJsonObject tc = findCase(id);

        QVERIFY2(
            !tc.isEmpty(),
            qPrintable(QString("No se encontro el caso JSON: %1").arg(id))
        );

        const QJsonObject in = tc["inputs"].toObject();
        const QJsonObject ex = tc["expected"].toObject();

        // ARRANGE
        BorneoConfig config;
        config.eslora = in["eslora"].toDouble();
        config.grilletes = in["grilletes"].toInt();
        config.profundidad = in["profundidad"].toDouble();

        // ACT
        const double radio = BorneoCalculator::calculateRadius(config);

        // ASSERT
        const double esperado = ex["radio"].toDouble();

        QVERIFY2(
            qAbs(radio - esperado) < kEps,
            qPrintable(
                QString("[%1] Radio esperado: %2 | Radio obtenido: %3")
                    .arg(id)
                    .arg(esperado, 0, 'f', 3)
                    .arg(radio, 0, 'f', 3)
            )
        );
    }
};

QTEST_APPLESS_MAIN(TestBorneoCalculator)

#include "test_borneocalculator.moc"