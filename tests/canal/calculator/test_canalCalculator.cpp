#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QtMath>
#include "canalCalculator.h"
#include "canalSessionState.h"

static constexpr double kEps = 0.01;

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/canalCalculator_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["canalCalculatorTestCases"].toArray();
}

class TestCanalCalculator : public QObject {
    Q_OBJECT
    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const {
        for (const QJsonValue& v : m_cases)
            if (v.toObject()["id"].toString() == id) return v.toObject();
        return {};
    }

private slots:
    void initTestCase() {
        m_cases = loadCases();
        QVERIFY2(!m_cases.isEmpty(), "JSON de casos no cargado. Revisa el .qrc");
    }

    // ── calculateCinematica ─────────────────────────────────────

    void test_CANCALC_01_Cinematica_Basica_Norte()               { runCinematicaTest("CANCALC_01_Cinematica_Basica_Norte"); }
    void test_CANCALC_02_Cinematica_Este_SinVelocidad()          { runCinematicaTest("CANCALC_02_Cinematica_Este_SinVelocidad"); }
    void test_CANCALC_03_Cinematica_ClampPositivo_RamaMayor180() { runCinematicaTest("CANCALC_03_Cinematica_ClampPositivo_RamaMayor180"); }
    void test_CANCALC_04_Cinematica_ClampNegativo_RamaMenorMenos180() { runCinematicaTest("CANCALC_04_Cinematica_ClampNegativo_RamaMenorMenos180"); }
    void test_CANCALC_05_Cinematica_SinClamp_ValorDiagonal()     { runCinematicaTest("CANCALC_05_Cinematica_SinClamp_ValorDiagonal"); }

    void test_CANCALC_06_ETA_Invalida_VelocidadCero() {
        QJsonObject tc = findCase("CANCALC_06_ETA_Invalida_VelocidadCero");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CanalSlot slot;
        CanalCalculator::calculateCinematica(
            QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), in["ownCourse"].toDouble(), in["ownSpeedDm"].toDouble(),
            QPointF(in["buoyX"].toDouble(), in["buoyY"].toDouble()), slot);

        QVERIFY2(qAbs(slot.distanciaYardas - ex["distanciaYardas"].toDouble()) < kEps,
                 qPrintable(QString("distanciaYardas: esperado %1, obtenido %2").arg(ex["distanciaYardas"].toDouble()).arg(slot.distanciaYardas)));
        QCOMPARE(slot.etaValid, ex["etaValid"].toBool());
        QVERIFY2(qAbs(slot.timeToArrivalMin - ex["timeToArrivalMin"].toDouble()) < kEps,
                 qPrintable(QString("timeToArrivalMin: esperado %1, obtenido %2").arg(ex["timeToArrivalMin"].toDouble()).arg(slot.timeToArrivalMin)));
    }

    void test_CANCALC_07_ETA_Invalida_DistanciaCero() {
        QJsonObject tc = findCase("CANCALC_07_ETA_Invalida_DistanciaCero");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CanalSlot slot;
        CanalCalculator::calculateCinematica(
            QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), in["ownCourse"].toDouble(), in["ownSpeedDm"].toDouble(),
            QPointF(in["buoyX"].toDouble(), in["buoyY"].toDouble()), slot);

        QVERIFY2(qAbs(slot.distanciaYardas - ex["distanciaYardas"].toDouble()) < kEps,
                 qPrintable(QString("distanciaYardas: esperado %1, obtenido %2").arg(ex["distanciaYardas"].toDouble()).arg(slot.distanciaYardas)));
        QCOMPARE(slot.etaValid, ex["etaValid"].toBool());
        QVERIFY2(qAbs(slot.timeToArrivalMin - ex["timeToArrivalMin"].toDouble()) < kEps,
                 qPrintable(QString("timeToArrivalMin: esperado %1, obtenido %2").arg(ex["timeToArrivalMin"].toDouble()).arg(slot.timeToArrivalMin)));
    }

    // ── isWithinAlarmWindow ──────────────────────────────────────

    void test_CANCALC_08_AlarmWindow_LimiteInferiorEstribor() { runAlarmWindowTest("CANCALC_08_AlarmWindow_LimiteInferiorEstribor"); }
    void test_CANCALC_09_AlarmWindow_JustoAntesEstribor()     { runAlarmWindowTest("CANCALC_09_AlarmWindow_JustoAntesEstribor"); }
    void test_CANCALC_10_AlarmWindow_CentroEstribor()         { runAlarmWindowTest("CANCALC_10_AlarmWindow_CentroEstribor"); }
    void test_CANCALC_11_AlarmWindow_LimiteSuperiorEstribor() { runAlarmWindowTest("CANCALC_11_AlarmWindow_LimiteSuperiorEstribor"); }
    void test_CANCALC_12_AlarmWindow_JustoDespuesEstribor()   { runAlarmWindowTest("CANCALC_12_AlarmWindow_JustoDespuesEstribor"); }
    void test_CANCALC_13_AlarmWindow_LimiteInferiorBabor()    { runAlarmWindowTest("CANCALC_13_AlarmWindow_LimiteInferiorBabor"); }
    void test_CANCALC_14_AlarmWindow_CentroBabor()            { runAlarmWindowTest("CANCALC_14_AlarmWindow_CentroBabor"); }
    void test_CANCALC_15_AlarmWindow_LimiteSuperiorBabor()    { runAlarmWindowTest("CANCALC_15_AlarmWindow_LimiteSuperiorBabor"); }
    void test_CANCALC_16_AlarmWindow_JustoDespuesBabor()      { runAlarmWindowTest("CANCALC_16_AlarmWindow_JustoDespuesBabor"); }
    void test_CANCALC_17_AlarmWindow_FueraDeRango_Popa()      { runAlarmWindowTest("CANCALC_17_AlarmWindow_FueraDeRango_Popa"); }
    void test_CANCALC_18_AlarmWindow_FueraDeRango_Proa()      { runAlarmWindowTest("CANCALC_18_AlarmWindow_FueraDeRango_Proa"); }

    // ── isBehind ─────────────────────────────────────────────────

    void test_CANCALC_19_IsBehind_LimiteInferior_NoIncluido()          { runIsBehindTest("CANCALC_19_IsBehind_LimiteInferior_NoIncluido"); }
    void test_CANCALC_20_IsBehind_JustoDespuesLimiteInferior()         { runIsBehindTest("CANCALC_20_IsBehind_JustoDespuesLimiteInferior"); }
    void test_CANCALC_21_IsBehind_CentroAstern()                       { runIsBehindTest("CANCALC_21_IsBehind_CentroAstern"); }
    void test_CANCALC_22_IsBehind_LimiteSuperior_NoIncluido()          { runIsBehindTest("CANCALC_22_IsBehind_LimiteSuperior_NoIncluido"); }
    void test_CANCALC_23_IsBehind_JustoAntesLimiteSuperior()           { runIsBehindTest("CANCALC_23_IsBehind_JustoAntesLimiteSuperior"); }
    void test_CANCALC_24_IsBehind_FueraDeRango_Proa()                  { runIsBehindTest("CANCALC_24_IsBehind_FueraDeRango_Proa"); }
    void test_CANCALC_25_IsBehind_FueraDeRango_HemisferioProaBabor()   { runIsBehindTest("CANCALC_25_IsBehind_FueraDeRango_HemisferioProaBabor"); }

    // ─────────────────────────────────────────────────────────────
    // Helpers compartidos
    // ─────────────────────────────────────────────────────────────

    void runCinematicaTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CanalSlot slot;
        CanalCalculator::calculateCinematica(
            QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), in["ownCourse"].toDouble(), in["ownSpeedDm"].toDouble(),
            QPointF(in["buoyX"].toDouble(), in["buoyY"].toDouble()), slot);

        QVERIFY2(qAbs(slot.distanciaYardas - ex["distanciaYardas"].toDouble()) < kEps,
                 qPrintable(QString("[%1] distanciaYardas: esperado %2, obtenido %3").arg(id).arg(ex["distanciaYardas"].toDouble()).arg(slot.distanciaYardas)));
        QVERIFY2(qAbs(slot.azimutVerdadero - ex["azimutVerdadero"].toDouble()) < 1.0,
                 qPrintable(QString("[%1] azimutVerdadero: esperado %2, obtenido %3").arg(id).arg(ex["azimutVerdadero"].toDouble()).arg(slot.azimutVerdadero)));
        QVERIFY2(qAbs(slot.rumboVerdadero - ex["rumboVerdadero"].toDouble()) < 1.0,
                 qPrintable(QString("[%1] rumboVerdadero: esperado %2, obtenido %3").arg(id).arg(ex["rumboVerdadero"].toDouble()).arg(slot.rumboVerdadero)));
        QVERIFY2(qAbs(slot.marcacionRelativa - ex["marcacionRelativa"].toDouble()) < 1.0,
                 qPrintable(QString("[%1] marcacionRelativa: esperado %2, obtenido %3").arg(id).arg(ex["marcacionRelativa"].toDouble()).arg(slot.marcacionRelativa)));
        QCOMPARE(slot.etaValid, ex["etaValid"].toBool());
        QVERIFY2(qAbs(slot.timeToArrivalMin - ex["timeToArrivalMin"].toDouble()) < kEps,
                 qPrintable(QString("[%1] timeToArrivalMin: esperado %2, obtenido %3").arg(id).arg(ex["timeToArrivalMin"].toDouble()).arg(slot.timeToArrivalMin)));
    }

    void runAlarmWindowTest(const QString& id) {
        QJsonObject tc = findCase(id);
        double bearing  = tc["inputs"].toObject()["relativeBearing"].toDouble();
        bool   expected = tc["expected"].toObject()["result"].toBool();

        bool result = CanalCalculator::isWithinAlarmWindow(bearing);

        QVERIFY2(result == expected,
                 qPrintable(QString("isWithinAlarmWindow(%1): esperado %2, obtenido %3")
                                .arg(bearing).arg(expected).arg(result)));
    }

    void runIsBehindTest(const QString& id) {
        QJsonObject tc = findCase(id);
        double bearing  = tc["inputs"].toObject()["relativeBearing"].toDouble();
        bool   expected = tc["expected"].toObject()["result"].toBool();

        bool result = CanalCalculator::isBehind(bearing);

        QVERIFY2(result == expected,
                 qPrintable(QString("isBehind(%1): esperado %2, obtenido %3")
                                .arg(bearing).arg(expected).arg(result)));
    }
};

QTEST_APPLESS_MAIN(TestCanalCalculator)
#include "test_canalCalculator.moc"
