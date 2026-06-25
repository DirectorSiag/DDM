#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QtMath>
#include "fondeoCalculator.h"
#include "model/fondeo/fondeoSessionState.h"

static constexpr double kEps = 0.01;

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/fondeoCalculator_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["fondeoCalculatorTestCases"].toArray();
}

class TestFondeoCalculator : public QObject {
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

    // ── resolvePuntoFondeo — Modo Track ───────────────────────────

    void test_CALC_01_ResolvePF_Track_Az90_Este() {
        QJsonObject tc = findCase("CALC_01_ResolvePF_Track_Az90_Este");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useTrack = true;
        cfg.trackAz  = in["trackAz"].toDouble();
        cfg.trackDt  = in["trackDt"].toDouble();
        QPointF trackPos(in["trackX"].toDouble(), in["trackY"].toDouble());

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, trackPos);

        QVERIFY2(qAbs(pf.x() - ex["pfX"].toDouble()) < kEps,
                 qPrintable(QString("pfX: esperado %1, obtenido %2").arg(ex["pfX"].toDouble()).arg(pf.x())));
        QVERIFY2(qAbs(pf.y() - ex["pfY"].toDouble()) < kEps,
                 qPrintable(QString("pfY: esperado %1, obtenido %2").arg(ex["pfY"].toDouble()).arg(pf.y())));
    }

    void test_CALC_02_ResolvePF_Track_Az0_Norte() {
        QJsonObject tc = findCase("CALC_02_ResolvePF_Track_Az0_Norte");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useTrack = true;
        cfg.trackAz  = in["trackAz"].toDouble();
        cfg.trackDt  = in["trackDt"].toDouble();
        QPointF trackPos(in["trackX"].toDouble(), in["trackY"].toDouble());

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, trackPos);

        QVERIFY2(qAbs(pf.x() - ex["pfX"].toDouble()) < kEps,
                 qPrintable(QString("pfX: esperado %1, obtenido %2").arg(ex["pfX"].toDouble()).arg(pf.x())));
        QVERIFY2(qAbs(pf.y() - ex["pfY"].toDouble()) < kEps,
                 qPrintable(QString("pfY: esperado %1, obtenido %2").arg(ex["pfY"].toDouble()).arg(pf.y())));
    }

    void test_CALC_03_ResolvePF_Track_Az180_Sur() {
        QJsonObject tc = findCase("CALC_03_ResolvePF_Track_Az180_Sur");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useTrack = true;
        cfg.trackAz  = in["trackAz"].toDouble();
        cfg.trackDt  = in["trackDt"].toDouble();
        QPointF trackPos(in["trackX"].toDouble(), in["trackY"].toDouble());

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, trackPos);

        QVERIFY2(qAbs(pf.x() - ex["pfX"].toDouble()) < kEps,
                 qPrintable(QString("pfX: esperado %1, obtenido %2").arg(ex["pfX"].toDouble()).arg(pf.x())));
        QVERIFY2(ex["pfYIsNegative"].toBool() ? pf.y() < 0.0 : true,
                 "pfY debe ser negativo para az=180°");
    }

    void test_CALC_04_ResolvePF_Track_DistanciaCero() {
        QJsonObject tc = findCase("CALC_04_ResolvePF_Track_DistanciaCero");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useTrack = true;
        cfg.trackAz  = in["trackAz"].toDouble();
        cfg.trackDt  = in["trackDt"].toDouble();
        QPointF trackPos(in["trackX"].toDouble(), in["trackY"].toDouble());

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, trackPos);

        QVERIFY2(qAbs(pf.x() - ex["pfX"].toDouble()) < kEps,
                 qPrintable(QString("pfX: esperado %1, obtenido %2").arg(ex["pfX"].toDouble()).arg(pf.x())));
        QVERIFY2(qAbs(pf.y() - ex["pfY"].toDouble()) < kEps,
                 qPrintable(QString("pfY: esperado %1, obtenido %2").arg(ex["pfY"].toDouble()).arg(pf.y())));
    }

    // ── resolvePuntoFondeo — Modo GMS ─────────────────────────────

    void test_CALC_05_ResolvePF_GMS_MismaPosicion() {
        QJsonObject tc = findCase("CALC_05_ResolvePF_GMS_MismaPosicion");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useGms   = true;
        cfg.pfLatDeg = in["pfLatDeg"].toInt(); cfg.pfLatMin = in["pfLatMin"].toInt(); cfg.pfLatSec = in["pfLatSec"].toDouble();
        cfg.pfLonDeg = in["pfLonDeg"].toInt(); cfg.pfLonMin = in["pfLonMin"].toInt(); cfg.pfLonSec = in["pfLonSec"].toDouble();

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, in["ownLat"].toDouble(), in["ownLon"].toDouble());

        QVERIFY2(qAbs(pf.x() - ex["pfX"].toDouble()) < kEps,
                 qPrintable(QString("pfX: esperado %1, obtenido %2").arg(ex["pfX"].toDouble()).arg(pf.x())));
        QVERIFY2(qAbs(pf.y() - ex["pfY"].toDouble()) < kEps,
                 qPrintable(QString("pfY: esperado %1, obtenido %2").arg(ex["pfY"].toDouble()).arg(pf.y())));
    }

    void test_CALC_06_ResolvePF_GMS_PfAlNorte() {
        QJsonObject tc = findCase("CALC_06_ResolvePF_GMS_PfAlNorte");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoConfig cfg;
        cfg.useGms   = true;
        cfg.pfLatDeg = in["pfLatDeg"].toInt(); cfg.pfLatMin = in["pfLatMin"].toInt(); cfg.pfLatSec = in["pfLatSec"].toDouble();
        cfg.pfLonDeg = in["pfLonDeg"].toInt(); cfg.pfLonMin = in["pfLonMin"].toInt(); cfg.pfLonSec = in["pfLonSec"].toDouble();

        QPointF pf = FondeoCalculator::resolvePuntoFondeo(cfg, in["ownLat"].toDouble(), in["ownLon"].toDouble());

        QVERIFY2(ex["pfXIsZero"].toBool() ? qAbs(pf.x()) < kEps : true,
                 qPrintable(QString("pfX debe ser ~0, obtenido %1").arg(pf.x())));
        QVERIFY2(ex["pfYIsPositive"].toBool() ? pf.y() > 0.0 : true,
                 "pfY debe ser positivo cuando PF está al norte");
        QVERIFY2(qAbs(pf.y() - ex["pfY"].toDouble()) < 0.1,
                 qPrintable(QString("pfY: esperado ~%1, obtenido %2").arg(ex["pfY"].toDouble()).arg(pf.y())));
    }

    // ── resolvePuntoAuxiliar ──────────────────────────────────────

    void test_CALC_07_ResolvePuntoAuxiliar_Az0_Norte() {
        QJsonObject tc = findCase("CALC_07_ResolvePuntoAuxiliar_Az0_Norte");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF pf(in["pfX"].toDouble(), in["pfY"].toDouble());
        QPointF pa = FondeoCalculator::resolvePuntoAuxiliar(pf, in["paAz"].toDouble(), in["paDt"].toDouble());

        QVERIFY2(qAbs(pa.x() - ex["paX"].toDouble()) < kEps,
                 qPrintable(QString("paX: esperado %1, obtenido %2").arg(ex["paX"].toDouble()).arg(pa.x())));
        QVERIFY2(qAbs(pa.y() - ex["paY"].toDouble()) < kEps,
                 qPrintable(QString("paY: esperado %1, obtenido %2").arg(ex["paY"].toDouble()).arg(pa.y())));
    }

    void test_CALC_08_ResolvePuntoAuxiliar_Az90_Este() {
        QJsonObject tc = findCase("CALC_08_ResolvePuntoAuxiliar_Az90_Este");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF pf(in["pfX"].toDouble(), in["pfY"].toDouble());
        QPointF pa = FondeoCalculator::resolvePuntoAuxiliar(pf, in["paAz"].toDouble(), in["paDt"].toDouble());

        QVERIFY2(ex["paXGreaterThanPfX"].toBool() ? pa.x() > pf.x() : true,
                 "paX debe ser mayor que pfX para az=90°");
        QVERIFY2(ex["paYEqualsPfY"].toBool() ? qAbs(pa.y() - pf.y()) < kEps : true,
                 qPrintable(QString("paY debe ser ≈pfY para az=90°, obtenido %1").arg(pa.y())));
    }

    void test_CALC_09_ResolvePuntoAuxiliar_Az180_Sur() {
        QJsonObject tc = findCase("CALC_09_ResolvePuntoAuxiliar_Az180_Sur");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF pf(in["pfX"].toDouble(), in["pfY"].toDouble());
        QPointF pa = FondeoCalculator::resolvePuntoAuxiliar(pf, in["paAz"].toDouble(), in["paDt"].toDouble());

        QVERIFY2(qAbs(pa.x() - ex["paX"].toDouble()) < kEps,
                 qPrintable(QString("paX: esperado %1, obtenido %2").arg(ex["paX"].toDouble()).arg(pa.x())));
        QVERIFY2(ex["paYIsNegative"].toBool() ? pa.y() < 0.0 : true,
                 "paY debe ser negativo para az=180°");
    }

    void test_CALC_10_ResolvePuntoAuxiliar_DistanciaCero() {
        QJsonObject tc = findCase("CALC_10_ResolvePuntoAuxiliar_DistanciaCero");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF pf(in["pfX"].toDouble(), in["pfY"].toDouble());
        QPointF pa = FondeoCalculator::resolvePuntoAuxiliar(pf, in["paAz"].toDouble(), in["paDt"].toDouble());

        QVERIFY2(qAbs(pa.x() - ex["paX"].toDouble()) < kEps,
                 qPrintable(QString("paX: esperado %1, obtenido %2").arg(ex["paX"].toDouble()).arg(pa.x())));
        QVERIFY2(qAbs(pa.y() - ex["paY"].toDouble()) < kEps,
                 qPrintable(QString("paY: esperado %1, obtenido %2").arg(ex["paY"].toDouble()).arg(pa.y())));
    }

    // ── calculateDistAzPfPa ───────────────────────────────────────

    void test_CALC_11_DistAzPfPa_Pitagoras() {
        QJsonObject tc = findCase("CALC_11_DistAzPfPa_Pitagoras");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.puntoFondeo   = QPointF(in["pfX"].toDouble(), in["pfY"].toDouble());
        state.puntoAuxiliar = QPointF(in["paX"].toDouble(), in["paY"].toDouble());

        FondeoCalculator::calculateDistAzPfPa(QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), state);

        QVERIFY2(qAbs(state.distanciaPF - ex["distanciaPF"].toDouble()) < 1.0,
                 qPrintable(QString("distanciaPF: esperado %1, obtenido %2").arg(ex["distanciaPF"].toDouble()).arg(state.distanciaPF)));
        QVERIFY2(qAbs(state.distanciaPA - ex["distanciaPA"].toDouble()) < 1.0,
                 qPrintable(QString("distanciaPA: esperado %1, obtenido %2").arg(ex["distanciaPA"].toDouble()).arg(state.distanciaPA)));
    }

    void test_CALC_12_DistAzPfPa_AzimutPF_Este() {
        QJsonObject tc = findCase("CALC_12_DistAzPfPa_AzimutPF_Este");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.puntoFondeo   = QPointF(in["pfX"].toDouble(), in["pfY"].toDouble());
        state.puntoAuxiliar = QPointF(in["paX"].toDouble(), in["paY"].toDouble());

        FondeoCalculator::calculateDistAzPfPa(QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), state);

        QVERIFY2(qAbs(state.azimutPF - ex["azimutPF"].toDouble()) < 1.0,
                 qPrintable(QString("azimutPF: esperado %1, obtenido %2").arg(ex["azimutPF"].toDouble()).arg(state.azimutPF)));
    }

    void test_CALC_13_DistAzPfPa_BPCoincideConPF() {
        QJsonObject tc = findCase("CALC_13_DistAzPfPa_BPCoincideConPF");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.puntoFondeo   = QPointF(in["pfX"].toDouble(), in["pfY"].toDouble());
        state.puntoAuxiliar = QPointF(in["paX"].toDouble(), in["paY"].toDouble());

        FondeoCalculator::calculateDistAzPfPa(QPointF(in["ownX"].toDouble(), in["ownY"].toDouble()), state);

        QVERIFY2(qAbs(state.distanciaPF - ex["distanciaPF"].toDouble()) < kEps,
                 qPrintable(QString("distanciaPF: esperado %1, obtenido %2").arg(ex["distanciaPF"].toDouble()).arg(state.distanciaPF)));
    }

    // ── calculateMarcacionRelativa ────────────────────────────────

    void test_CALC_14_MarcacionRelativa_FasePA() {
        QJsonObject tc = findCase("CALC_14_MarcacionRelativa_FasePA");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.paAlcanzado    = in["paAlcanzado"].toBool();
        state.azimutPA       = in["azimutPA"].toDouble();
        state.distanciaPA    = in["distanciaPA"].toDouble();
        state.azimutPF       = in["azimutPF"].toDouble();
        state.distanciaPF    = in["distanciaPF"].toDouble();

        FondeoCalculator::calculateMarcacionRelativa(in["ownCourse"].toDouble(), state);

        QVERIFY2(qAbs(state.azimutRelativo - ex["azimutRelativo"].toDouble()) < kEps,
                 qPrintable(QString("azimutRelativo: esperado %1, obtenido %2").arg(ex["azimutRelativo"].toDouble()).arg(state.azimutRelativo)));
        QVERIFY2(qAbs(state.distanciaRelativa - ex["distanciaRelativa"].toDouble()) < kEps,
                 qPrintable(QString("distanciaRelativa: esperado %1, obtenido %2").arg(ex["distanciaRelativa"].toDouble()).arg(state.distanciaRelativa)));
    }

    void test_CALC_15_MarcacionRelativa_FasePF() {
        QJsonObject tc = findCase("CALC_15_MarcacionRelativa_FasePF");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.paAlcanzado    = in["paAlcanzado"].toBool();
        state.azimutPA       = in["azimutPA"].toDouble();
        state.distanciaPA    = in["distanciaPA"].toDouble();
        state.azimutPF       = in["azimutPF"].toDouble();
        state.distanciaPF    = in["distanciaPF"].toDouble();

        FondeoCalculator::calculateMarcacionRelativa(in["ownCourse"].toDouble(), state);

        QVERIFY2(qAbs(state.azimutRelativo - ex["azimutRelativo"].toDouble()) < kEps,
                 qPrintable(QString("azimutRelativo: esperado %1, obtenido %2").arg(ex["azimutRelativo"].toDouble()).arg(state.azimutRelativo)));
        QVERIFY2(qAbs(state.distanciaRelativa - ex["distanciaRelativa"].toDouble()) < kEps,
                 qPrintable(QString("distanciaRelativa: esperado %1, obtenido %2").arg(ex["distanciaRelativa"].toDouble()).arg(state.distanciaRelativa)));
    }

    void test_CALC_16_MarcacionRelativa_WrapAround() {
        QJsonObject tc = findCase("CALC_16_MarcacionRelativa_WrapAround");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.paAlcanzado = in["paAlcanzado"].toBool();
        state.azimutPA    = in["azimutPA"].toDouble();
        state.distanciaPA = in["distanciaPA"].toDouble();

        FondeoCalculator::calculateMarcacionRelativa(in["ownCourse"].toDouble(), state);

        QVERIFY2(qAbs(state.azimutRelativo - ex["azimutRelativo"].toDouble()) < kEps,
                 qPrintable(QString("azimutRelativo: esperado %1, obtenido %2").arg(ex["azimutRelativo"].toDouble()).arg(state.azimutRelativo)));
    }

    void test_CALC_17_MarcacionRelativa_ObjetivoEnProa() {
        QJsonObject tc = findCase("CALC_17_MarcacionRelativa_ObjetivoEnProa");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.paAlcanzado = in["paAlcanzado"].toBool();
        state.azimutPA    = in["azimutPA"].toDouble();
        state.distanciaPA = in["distanciaPA"].toDouble();

        FondeoCalculator::calculateMarcacionRelativa(in["ownCourse"].toDouble(), state);

        QVERIFY2(qAbs(state.azimutRelativo - ex["azimutRelativo"].toDouble()) < kEps,
                 qPrintable(QString("azimutRelativo: esperado %1, obtenido %2").arg(ex["azimutRelativo"].toDouble()).arg(state.azimutRelativo)));
    }

    // ── calculatePanelPredictivo ──────────────────────────────────

    void test_CALC_18_Panel_Banda1_AdelanteToda()       { runPanelTest("CALC_18_Panel_Banda1_AdelanteToda"); }
    void test_CALC_19_Panel_Banda2_AdelanteMedia()      { runPanelTest("CALC_19_Panel_Banda2_AdelanteMedia"); }
    void test_CALC_20_Panel_Banda3_AdelanteDespacio()   { runPanelTest("CALC_20_Panel_Banda3_AdelanteDespacio"); }
    void test_CALC_21_Panel_Banda4_ParaMaquinas()       { runPanelTest("CALC_21_Panel_Banda4_ParaMaquinas"); }
    void test_CALC_22_Panel_Banda5_MaquinasAtras()      { runPanelTest("CALC_22_Panel_Banda5_MaquinasAtras"); }
    void test_CALC_23_Panel_Banda6_Detencion()          { runPanelTest("CALC_23_Panel_Banda6_Detencion"); }
    void test_CALC_24_Panel_BordeExacto_R1()            { runPanelTest("CALC_24_Panel_BordeExacto_R1"); }
    void test_CALC_25_Panel_BordeExacto_R5()            { runPanelTest("CALC_25_Panel_BordeExacto_R5"); }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para los casos de Panel Predictivo
    // ─────────────────────────────────────────────────────────────
    void runPanelTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        FondeoSessionState state;
        state.distanciaPF    = in["distanciaPF"].toDouble();
        state.config.r1 = in["r1"].toDouble();
        state.config.r2 = in["r2"].toDouble();
        state.config.r3 = in["r3"].toDouble();
        state.config.r4 = in["r4"].toDouble();
        state.config.r5 = in["r5"].toDouble();

        FondeoCalculator::calculatePanelPredictivo(state);

        QCOMPARE(state.movimientoActual,  ex["movimientoActual"].toString());
        QCOMPARE(state.proximoMovimiento, ex["proximoMovimiento"].toString());
    }
};

QTEST_APPLESS_MAIN(TestFondeoCalculator)
#include "test_fondeoCalculator.moc"
