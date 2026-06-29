#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <cmath>

#include "haCalculator.h"
#include "haSessionState.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static HaSessionState runCalculator(const QJsonObject& tc)
{
    QPointF ownPos(tc["ownX"].toDouble(), tc["ownY"].toDouble());
    QPointF fallPoint(tc["fallX"].toDouble(), tc["fallY"].toDouble());
    double ownCourse = tc["ownCourse"].toDouble();
    double ownSpeed  = tc["ownSpeed"].toDouble();

    HaSessionState state;
    HaCalculator::calculate(ownPos, ownCourse, ownSpeed, fallPoint, state);
    return state;
}

static QJsonArray loadCases()
{
    QFile file(":/json/json/hacalculator_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
               .object()["haCalculatorTestCases"].toArray();
}

// ─────────────────────────────────────────────────────────────────────────────
// Suite
// ─────────────────────────────────────────────────────────────────────────────

class TestHaCalculator : public QObject
{
    Q_OBJECT

private:
    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const {
        for (const QJsonValue& v : m_cases)
            if (v.toObject()["id"].toString() == id)
                return v.toObject();
        return {};
    }

private slots:

    void initTestCase() {
        m_cases = loadCases();
        QVERIFY2(!m_cases.isEmpty(), "No se pudo cargar hacalculator_cases.json. Revisá el .qrc");
    }

    void test_CALC_01_AzimutVerdadero_PuntoAlEste() {
        QJsonObject tc = findCase("CALC_01_AzimutVerdadero_PuntoAlEste");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.trueAzimuthDeg - exp["trueAzimuthDeg_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("trueAzimuthDeg=%1 esperado ~90°").arg(s.trueAzimuthDeg)));
    }

    void test_CALC_02_AzimutVerdadero_PuntoAlNorte() {
        QJsonObject tc = findCase("CALC_02_AzimutVerdadero_PuntoAlNorte");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        double diff = std::min(std::abs(s.trueAzimuthDeg - 0.0), std::abs(s.trueAzimuthDeg - 360.0));
        QVERIFY2(diff < exp["tolerance"].toDouble(),
                 qPrintable(QString("trueAzimuthDeg=%1 esperado ~0°").arg(s.trueAzimuthDeg)));
    }

    void test_CALC_03_AzimutVerdadero_PuntoAlSur() {
        QJsonObject tc = findCase("CALC_03_AzimutVerdadero_PuntoAlSur");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.trueAzimuthDeg - exp["trueAzimuthDeg_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("trueAzimuthDeg=%1 esperado ~180°").arg(s.trueAzimuthDeg)));
    }

    void test_CALC_04_Distancia_Correcta() {
        QJsonObject tc = findCase("CALC_04_Distancia_Correcta");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.distanceYards - exp["distanceYards"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("distanceYards=%1 esperado 10000").arg(s.distanceYards)));
    }

    void test_CALC_05_MarcacionRelativa_PuntoEnProa() {
        QJsonObject tc = findCase("CALC_05_MarcacionRelativa_PuntoEnProa");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.relativeBearingDeg - exp["relativeBearingDeg_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("relativeBearingDeg=%1 esperado ~0°").arg(s.relativeBearingDeg)));
        QCOMPARE(s.banda, exp["banda"].toString());
    }

    void test_CALC_06_MarcacionRelativa_PuntoEnEstribor() {
        QJsonObject tc = findCase("CALC_06_MarcacionRelativa_PuntoEnEstribor");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.relativeBearingDeg - exp["relativeBearingDeg_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("relativeBearingDeg=%1 esperado ~90°").arg(s.relativeBearingDeg)));
        QCOMPARE(s.banda, exp["banda"].toString());
    }

    void test_CALC_07_MarcacionRelativa_PuntoEnBabor() {
        QJsonObject tc = findCase("CALC_07_MarcacionRelativa_PuntoEnBabor");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QVERIFY2(std::abs(s.relativeBearingDeg - exp["relativeBearingDeg_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("relativeBearingDeg=%1 esperado ~90°").arg(s.relativeBearingDeg)));
        QCOMPARE(s.banda, exp["banda"].toString());
    }

    void test_CALC_08_ETA_Valido_VelocidadPositiva() {
        QJsonObject tc = findCase("CALC_08_ETA_Valido_VelocidadPositiva");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.etaValid, exp["etaValid"].toBool());
        QVERIFY2(s.timeToArrivalMin > 0.0, "ETA debe ser mayor que 0");
    }

    void test_CALC_09_ETA_Invalido_VelocidadCero() {
        QJsonObject tc = findCase("CALC_09_ETA_Invalido_VelocidadCero");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.etaValid, exp["etaValid"].toBool());
        QCOMPARE(s.timeToArrivalMin, exp["timeToArrivalMin"].toDouble());
    }

    void test_CALC_10_ETA_Invalido_DistanciaCero() {
        QJsonObject tc = findCase("CALC_10_ETA_Invalido_DistanciaCero");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.etaValid, exp["etaValid"].toBool());
        QCOMPARE(s.timeToArrivalMin, exp["timeToArrivalMin"].toDouble());
    }

    void test_CALC_11_ETA_Calculo_Correcto() {
        QJsonObject tc = findCase("CALC_11_ETA_Calculo_Correcto");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.etaValid, exp["etaValid"].toBool());
        QVERIFY2(std::abs(s.timeToArrivalMin - exp["timeToArrivalMin_approx"].toDouble()) < exp["tolerance"].toDouble(),
                 qPrintable(QString("timeToArrivalMin=%1 esperado ~30.0").arg(s.timeToArrivalMin)));
    }

    void test_CALC_12_IconCenter_EsElPuntoDeCaida() {
        QJsonObject tc = findCase("CALC_12_IconCenter_EsElPuntoDeCaida");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.haIconCenter.x(), exp["haIconCenterX"].toDouble());
        QCOMPARE(s.haIconCenter.y(), exp["haIconCenterY"].toDouble());
    }

    void test_CALC_13_Banda_Popa() {
        QJsonObject tc = findCase("CALC_13_Banda_Popa");
        HaSessionState s = runCalculator(tc);
        QJsonObject exp = tc["expected"].toObject();
        QCOMPARE(s.banda, exp["banda"].toString());
    }
};

QTEST_APPLESS_MAIN(TestHaCalculator)
#include "test_hacalculator.moc"
