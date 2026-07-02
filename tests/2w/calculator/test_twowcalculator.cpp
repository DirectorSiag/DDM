#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <cmath>

#include "twoWCalculator.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Ejecuta TwoWCalculator::calculate con los parámetros del caso JSON y
// devuelve todos los outputs empaquetados en una struct para no repetir el
// bloque de llamada en cada test.
struct CalcOutputs {
    QPointF         guideCenter;
    QPointF         ownCenter;
    QList<QPointF>  allyCenters;
    bool            etaValid      = false;
    double          courseDeg     = 0.0;
    double          etaMin        = 0.0;
    double          currentAzDeg  = 0.0;
    double          currentDistNm = 0.0;
    double          expectedAzDeg = 0.0;
    double          expectedDistNm= 0.0;
};

static CalcOutputs runCalculator(const QJsonObject& tc)
{
    QPointF guidePos(tc["guideX"].toDouble(), tc["guideY"].toDouble());
    QPointF ownPos  (tc["ownX"].toDouble(),   tc["ownY"].toDouble());
    double  ownSpeed      = tc["ownSpeed"].toDouble();
    int     bpStation     = tc["bpStation"].toInt();
    double  circleRadius  = tc["circleRadiusNm"].toDouble();

    QList<int> selectedStations;
    for (const QJsonValue& v : tc["selectedStations"].toArray())
        selectedStations.append(v.toInt());

    CalcOutputs out;
    TwoWCalculator::calculate(
        guidePos,
        ownPos, ownSpeed,
        bpStation, circleRadius, selectedStations,
        out.guideCenter,   out.ownCenter,     out.allyCenters,
        out.etaValid,      out.courseDeg,     out.etaMin,
        out.currentAzDeg,  out.currentDistNm,
        out.expectedAzDeg, out.expectedDistNm
    );
    return out;
}

static void verifyNear(double actual, double expected, double tolerance, const QString& label)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance,
             qPrintable(QString("%1=%2 no coincide con esperado=%3 tolerancia=%4")
                        .arg(label).arg(actual).arg(expected).arg(tolerance)));
}

static void verifyPointNear(const QPointF& actual, const QJsonObject& expected, double tolerance, const QString& label)
{
    verifyNear(actual.x(), expected["x"].toDouble(), tolerance, label + ".x");
    verifyNear(actual.y(), expected["y"].toDouble(), tolerance, label + ".y");
}

// Carga el JSON de recursos y devuelve el array de casos.
static QJsonArray loadCases()
{
    QFile file(":/json/json/twowcalculator_cases.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll())
               .object()["twoWCalculatorTestCases"]
               .toArray();
}

// ─────────────────────────────────────────────────────────────────────────────
// Suite
// ─────────────────────────────────────────────────────────────────────────────
class TestTwoWCalculator : public QObject
{
    Q_OBJECT

private:
    QJsonArray m_cases;

    // Busca un caso por id dentro del array cargado.
    QJsonObject findCase(const QString& id) const {
        for (const QJsonValue& v : m_cases)
            if (v.toObject()["id"].toString() == id)
                return v.toObject();
        return {};
    }

private slots:

    // Se ejecuta una sola vez antes de todos los tests.
    void initTestCase() {
        m_cases = loadCases();
        QVERIFY2(!m_cases.isEmpty(), "twowcalculator_cases.json must not be empty");

        const QStringList requiredIds = {
            QStringLiteral("CALC_01_GuideCenter_EsLaPosicionDelGuia"),
            QStringLiteral("CALC_02_EstacionProyectada_AlOeste_Azimut270"),
            QStringLiteral("CALC_03_EstacionProyectada_AlNorte_Azimut0"),
            QStringLiteral("CALC_04_Radio_Escala_Duplicado"),
            QStringLiteral("CALC_05_ETA_Invalida_VelocidadCero"),
            QStringLiteral("CALC_06_ETA_Valida_VelocidadPositiva"),
            QStringLiteral("CALC_07_DistanciaActual_Correcta"),
            QStringLiteral("CALC_08_AzimutActual_BPAlSurDelGuia"),
            QStringLiteral("CALC_09_AzimutActual_BPAlNorteDelGuia"),
            QStringLiteral("CALC_10_Aliados_ListaVacia"),
            QStringLiteral("CALC_11_Aliados_TresEstaciones"),
            QStringLiteral("CALC_12_ValoresEsperados_MatchTablaA"),
            QStringLiteral("CALC_13_CentroAliado_CorrectamenteProyectado")
        };

        const QStringList numericInputs = {
            QStringLiteral("guideX"),
            QStringLiteral("guideY"),
            QStringLiteral("ownX"),
            QStringLiteral("ownY"),
            QStringLiteral("ownSpeed"),
            QStringLiteral("bpStation"),
            QStringLiteral("circleRadiusNm")
        };

        QSet<QString> seenIds;
        for (const QJsonValue& value : m_cases) {
            QVERIFY2(value.isObject(), "Each calculator case must be a JSON object");

            const QJsonObject tc = value.toObject();
            const QString id = tc["id"].toString();
            QVERIFY2(!id.isEmpty(), "Each calculator case must define a non-empty id");
            QVERIFY2(!seenIds.contains(id),
                     qPrintable(QStringLiteral("Duplicated calculator case id: %1").arg(id)));
            seenIds.insert(id);

            for (const QString& key : numericInputs) {
                QVERIFY2(tc[key].isDouble(),
                         qPrintable(QStringLiteral("Case %1 must define numeric %2").arg(id, key)));
            }

            QVERIFY2(tc["selectedStations"].isArray(),
                     qPrintable(QStringLiteral("Case %1 must define selectedStations array").arg(id)));

            const QJsonObject exp = tc["expected"].toObject();
            QVERIFY2(!exp.isEmpty(),
                     qPrintable(QStringLiteral("Case %1 must define expected values").arg(id)));
        }

        for (const QString& id : requiredIds) {
            QVERIFY2(seenIds.contains(id),
                     qPrintable(QStringLiteral("Missing calculator case id: %1").arg(id)));
        }
    }

    // ── CALC_01 ──────────────────────────────────────────────────────────────
    void test_CALC_01_GuideCenter_EsLaPosicionDelGuia() {
        QJsonObject tc  = findCase("CALC_01_GuideCenter_EsLaPosicionDelGuia");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.guideCenter.x(), exp["guideCenterX"].toDouble());
        QCOMPARE(out.guideCenter.y(), exp["guideCenterY"].toDouble());
    }

    // ── CALC_02 ──────────────────────────────────────────────────────────────
    void test_CALC_02_EstacionProyectada_AlOeste_Azimut270() {
        QJsonObject tc  = findCase("CALC_02_EstacionProyectada_AlOeste_Azimut270");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        const double tolerance = exp["tolerance"].toDouble();
        verifyNear(out.ownCenter.x(), exp["ownCenterX"].toDouble(), tolerance, "ownCenter.x");
        verifyNear(out.ownCenter.y(), exp["ownCenterY"].toDouble(), tolerance, "ownCenter.y");

    }

    // ── CALC_03 ──────────────────────────────────────────────────────────────
    void test_CALC_03_EstacionProyectada_AlNorte_Azimut0() {
        QJsonObject tc  = findCase("CALC_03_EstacionProyectada_AlNorte_Azimut0");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        const double tolerance = exp["tolerance"].toDouble();
        verifyNear(out.ownCenter.x(), exp["ownCenterX"].toDouble(), tolerance, "ownCenter.x");
        verifyNear(out.ownCenter.y(), exp["ownCenterY"].toDouble(), tolerance, "ownCenter.y");

    }

    // ── CALC_04 ──────────────────────────────────────────────────────────────
    void test_CALC_04_Radio_Escala_Duplicado() {
        // Obtenemos el caso base (radio 1.0)
        QJsonObject tc = findCase("CALC_04_Radio_Escala_Duplicado");
        QJsonObject exp = tc["expected"].toObject();

        // Radio 1.0 — usamos el caso base con radio 1.0 explícito
        QJsonObject tc1 = tc;
        tc1["circleRadiusNm"] = 1.0;
        CalcOutputs out1 = runCalculator(tc1);

        // Radio 2.0 — mismo caso pero con radio 2.0
        CalcOutputs out2 = runCalculator(tc);  // ya tiene circleRadiusNm=2.0

        // La distancia desde el Guía (0,0) al centro de la estación debe ser el doble
        double dist1 = std::sqrt(out1.ownCenter.x() * out1.ownCenter.x() +
                                  out1.ownCenter.y() * out1.ownCenter.y());
        double dist2 = std::sqrt(out2.ownCenter.x() * out2.ownCenter.x() +
                                  out2.ownCenter.y() * out2.ownCenter.y());

        const double expectedScale = exp["distanceScaleFactor"].toDouble();
        const double tolerance = exp["tolerance"].toDouble();
        verifyNear(dist2, expectedScale * dist1, tolerance, "scaled station distance");

    }

    // ── CALC_05 ──────────────────────────────────────────────────────────────
    void test_CALC_05_ETA_Invalida_VelocidadCero() {
        QJsonObject tc  = findCase("CALC_05_ETA_Invalida_VelocidadCero");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.etaValid, exp["etaValid"].toBool());
        QCOMPARE(out.etaMin,   exp["etaMin"].toDouble());
    }

    // ── CALC_06 ──────────────────────────────────────────────────────────────
    void test_CALC_06_ETA_Valida_VelocidadPositiva() {
        QJsonObject tc  = findCase("CALC_06_ETA_Valida_VelocidadPositiva");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.etaValid, exp["etaValid"].toBool());
        verifyNear(out.etaMin,
                   exp["etaMin_approx"].toDouble(),
                   exp["tolerance"].toDouble(),
                   "etaMin");
    }

    // ── CALC_07 ──────────────────────────────────────────────────────────────
    void test_CALC_07_DistanciaActual_Correcta() {
        QJsonObject tc  = findCase("CALC_07_DistanciaActual_Correcta");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        double expected  = exp["currentDistNm_approx"].toDouble();
        double tolerance = exp["tolerance"].toDouble();

        QVERIFY2(std::abs(out.currentDistNm - expected) < tolerance,
                 qPrintable(QString("currentDistNm=%1 no está dentro de tolerancia %2 de %3")
                                .arg(out.currentDistNm).arg(tolerance).arg(expected)));
    }

    // ── CALC_08 ──────────────────────────────────────────────────────────────
    void test_CALC_08_AzimutActual_BPAlSurDelGuia() {
        QJsonObject tc  = findCase("CALC_08_AzimutActual_BPAlSurDelGuia");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        double expected  = exp["currentAzDeg_approx"].toDouble();
        double tolerance = exp["tolerance"].toDouble();

        QVERIFY2(std::abs(out.currentAzDeg - expected) < tolerance,
                 qPrintable(QString("currentAzDeg=%1 no es ~%2° (Sur)")
                                .arg(out.currentAzDeg).arg(expected)));
    }

    // ── CALC_09 ──────────────────────────────────────────────────────────────
    void test_CALC_09_AzimutActual_BPAlNorteDelGuia() {
        QJsonObject tc  = findCase("CALC_09_AzimutActual_BPAlNorteDelGuia");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        double expected  = exp["currentAzDeg_approx"].toDouble();
        double tolerance = exp["tolerance"].toDouble();

        // Ángulo 0° puede devolver 0 o 360, ambos son correctos
        double diff = std::min(std::abs(out.currentAzDeg - expected),
                               std::abs(out.currentAzDeg - 360.0));

        QVERIFY2(diff < tolerance,
                 qPrintable(QString("currentAzDeg=%1 no es ~0° (Norte)")
                                .arg(out.currentAzDeg)));
    }

    // ── CALC_10 ──────────────────────────────────────────────────────────────
    void test_CALC_10_Aliados_ListaVacia() {
        QJsonObject tc  = findCase("CALC_10_Aliados_ListaVacia");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.allyCenters.size(), exp["allyCentersCount"].toInt());
    }

    // ── CALC_11 ──────────────────────────────────────────────────────────────
    void test_CALC_11_Aliados_TresEstaciones() {
        QJsonObject tc  = findCase("CALC_11_Aliados_TresEstaciones");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.allyCenters.size(), exp["allyCentersCount"].toInt());

        const QJsonArray expectedCenters = exp["allyCenters"].toArray();
        QCOMPARE(out.allyCenters.size(), expectedCenters.size());

        const double tolerance = exp["tolerance"].toDouble();
        for (int i = 0; i < expectedCenters.size(); ++i) {
            verifyPointNear(out.allyCenters[i],
                            expectedCenters[i].toObject(),
                            tolerance,
                            QStringLiteral("allyCenters[%1]").arg(i));
        }
    }

    // ── CALC_12 ──────────────────────────────────────────────────────────────
    void test_CALC_12_ValoresEsperados_MatchTablaA() {
        QJsonObject tc  = findCase("CALC_12_ValoresEsperados_MatchTablaA");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        QCOMPARE(out.expectedAzDeg,  exp["expectedAzDeg"].toDouble());
        QCOMPARE(out.expectedDistNm, exp["expectedDistNm"].toDouble());
    }
    // ── CALC_13 ──────────────────────────────────────────────────────────────
    void test_CALC_13_CentroAliado_CorrectamenteProyectado() {
        QJsonObject tc  = findCase("CALC_13_CentroAliado_CorrectamenteProyectado");
        QJsonObject exp = tc["expected"].toObject();
        CalcOutputs out = runCalculator(tc);

        // Verificamos que realmente se haya proyectado un aliado
        QVERIFY2(out.allyCenters.size() == 1, "La lista de aliados no tiene exactamente 1 elemento");

        double expectedX = exp["allyCenterX"].toDouble();
        double expectedY = exp["allyCenterY"].toDouble();
        double tolerance = exp["tolerance"].toDouble();

        // Verificamos la precisión trigonométrica y la conversión a DM
        QVERIFY2(std::abs(out.allyCenters[0].x() - expectedX) < tolerance,
                 qPrintable(QString("El X del aliado (%1) no coincide con el esperado (%2)")
                                .arg(out.allyCenters[0].x()).arg(expectedX)));

        QVERIFY2(std::abs(out.allyCenters[0].y() - expectedY) < tolerance,
                 qPrintable(QString("El Y del aliado (%1) no coincide con el esperado (%2)")
                                .arg(out.allyCenters[0].y()).arg(expectedY)));
    }
};

QTEST_APPLESS_MAIN(TestTwoWCalculator)
#include "test_twowcalculator.moc"
