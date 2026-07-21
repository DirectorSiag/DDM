#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QtMath>
#include "textCalculator.h"

static constexpr double kEps = 0.01;

static QJsonArray loadCases()
{
    QFile file(":/json/json/textCalculator_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["textCalculatorTestCases"].toArray();
}

class TestTextCalculator : public QObject {
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

    // ── resolveFromBearing ─────────────────────────────────────────

    void test_CALC_01_ResolveBearing_Az90_Este_Verdadero() { runBearingCase("CALC_01_ResolveBearing_Az90_Este_Verdadero"); }
    void test_CALC_02_ResolveBearing_Az0_Norte_Verdadero()  { runBearingCase("CALC_02_ResolveBearing_Az0_Norte_Verdadero"); }
    void test_CALC_04_ResolveBearing_DistanciaCero()        { runBearingCase("CALC_04_ResolveBearing_DistanciaCero"); }
    void test_CALC_05_ResolveBearing_Relativo_SumaCourse()  { runBearingCase("CALC_05_ResolveBearing_Relativo_SumaCourse"); }
    void test_CALC_06_ResolveBearing_Relativo_WrapAround()  { runBearingCase("CALC_06_ResolveBearing_Relativo_WrapAround"); }

    void test_CALC_03_ResolveBearing_Az180_Sur_Verdadero() {
        QJsonObject tc = findCase("CALC_03_ResolveBearing_Az180_Sur_Verdadero");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF ref(in["refX"].toDouble(), in["refY"].toDouble());
        QPointF r = TextCalculator::resolveFromBearing(ref, in["az"].toDouble(), in["dt"].toDouble(),
                                                         in["verdadero"].toBool(), in["ownCourse"].toDouble());

        QVERIFY2(qAbs(r.x() - ex["x"].toDouble()) < kEps,
                 qPrintable(QString("x: esperado %1, obtenido %2").arg(ex["x"].toDouble()).arg(r.x())));
        QVERIFY2(ex["yIsNegative"].toBool() ? r.y() < 0.0 : true, "y debe ser negativo para az=180°");
    }

    void runBearingCase(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF ref(in["refX"].toDouble(), in["refY"].toDouble());
        QPointF r = TextCalculator::resolveFromBearing(ref, in["az"].toDouble(), in["dt"].toDouble(),
                                                         in["verdadero"].toBool(), in["ownCourse"].toDouble());

        QVERIFY2(qAbs(r.x() - ex["x"].toDouble()) < kEps,
                 qPrintable(QString("[%1] x: esperado %2, obtenido %3").arg(id).arg(ex["x"].toDouble()).arg(r.x())));
        QVERIFY2(qAbs(r.y() - ex["y"].toDouble()) < kEps,
                 qPrintable(QString("[%1] y: esperado %2, obtenido %3").arg(id).arg(ex["y"].toDouble()).arg(r.y())));
    }

    // ── resolveFromLatLon ──────────────────────────────────────────

    void test_CALC_07_ResolveLatLon_MismaPosicion() {
        QJsonObject tc = findCase("CALC_07_ResolveLatLon_MismaPosicion");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF r = TextCalculator::resolveFromLatLon(in["originLat"].toDouble(), in["originLon"].toDouble(),
                                                        in["targetLat"].toDouble(), in["targetLon"].toDouble());

        QVERIFY2(qAbs(r.x() - ex["x"].toDouble()) < kEps, "x debe ser ~0");
        QVERIFY2(qAbs(r.y() - ex["y"].toDouble()) < kEps, "y debe ser ~0");
    }

    void test_CALC_08_ResolveLatLon_TargetAlNorte() {
        QJsonObject tc = findCase("CALC_08_ResolveLatLon_TargetAlNorte");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF r = TextCalculator::resolveFromLatLon(in["originLat"].toDouble(), in["originLon"].toDouble(),
                                                        in["targetLat"].toDouble(), in["targetLon"].toDouble());

        QVERIFY2(ex["xIsZero"].toBool() ? qAbs(r.x()) < kEps : true, "x debe ser ~0");
        QVERIFY2(ex["yIsPositive"].toBool() ? r.y() > 0.0 : true, "y debe ser positivo (target al norte)");
    }

    void test_CALC_09_ResolveLatLon_TargetAlEste() {
        QJsonObject tc = findCase("CALC_09_ResolveLatLon_TargetAlEste");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF r = TextCalculator::resolveFromLatLon(in["originLat"].toDouble(), in["originLon"].toDouble(),
                                                        in["targetLat"].toDouble(), in["targetLon"].toDouble());

        QVERIFY2(ex["yIsZero"].toBool() ? qAbs(r.y()) < kEps : true, "y debe ser ~0");
        QVERIFY2(ex["xIsPositive"].toBool() ? r.x() > 0.0 : true, "x debe ser positivo (target al este)");
    }

    // ── resolveAssociatedPosition ─────────────────────────────────

    void test_CALC_10_ResolveAssociated_OffsetPositivo() { runAssociatedCase("CALC_10_ResolveAssociated_OffsetPositivo"); }
    void test_CALC_11_ResolveAssociated_OffsetCero()     { runAssociatedCase("CALC_11_ResolveAssociated_OffsetCero"); }
    void test_CALC_12_ResolveAssociated_OffsetNegativo() { runAssociatedCase("CALC_12_ResolveAssociated_OffsetNegativo"); }

    void runAssociatedCase(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        QPointF trackPos(in["trackX"].toDouble(), in["trackY"].toDouble());
        QPointF r = TextCalculator::resolveAssociatedPosition(trackPos, in["offsetX"].toDouble(), in["offsetY"].toDouble());

        QVERIFY2(qAbs(r.x() - ex["x"].toDouble()) < kEps,
                 qPrintable(QString("[%1] x: esperado %2, obtenido %3").arg(id).arg(ex["x"].toDouble()).arg(r.x())));
        QVERIFY2(qAbs(r.y() - ex["y"].toDouble()) < kEps,
                 qPrintable(QString("[%1] y: esperado %2, obtenido %3").arg(id).arg(ex["y"].toDouble()).arg(r.y())));
    }
};

QTEST_APPLESS_MAIN(TestTextCalculator)
#include "test_textCalculator.moc"