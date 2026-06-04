#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "twoWStationTable.h"


static QJsonArray loadCases()
{
    QFile file(":/json/json/twowstationtable_cases.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll())
               .object()["twoWStationTableTestCases"]
               .toArray();
}

class TestTwoWStationTable : public QObject
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
        QVERIFY2(!m_cases.isEmpty(),
                 "No se pudo cargar twowstationtable_cases.json. Revisá el .qrc");
    }

    // ── TBL_01 ───────────────────────────────────────────────────────────────
    void test_TBL_01_Estacion1_ValoresCorrectos() {
        QJsonObject tc  = findCase("TBL_01_Estacion1_ValoresCorrectos");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_02 ───────────────────────────────────────────────────────────────
    void test_TBL_02_Estacion7_ValoresCorrectos() {
        QJsonObject tc  = findCase("TBL_02_Estacion7_ValoresCorrectos");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_03 ───────────────────────────────────────────────────────────────
    void test_TBL_03_Estacion12_ValoresCorrectos() {
        QJsonObject tc  = findCase("TBL_03_Estacion12_ValoresCorrectos");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_04 ───────────────────────────────────────────────────────────────
    void test_TBL_04_Estacion68_ValoresCorrectos() {
        QJsonObject tc  = findCase("TBL_04_Estacion68_ValoresCorrectos");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_05 ───────────────────────────────────────────────────────────────
    void test_TBL_05_Estacion26_NoPresente() {
        QJsonObject tc  = findCase("TBL_05_Estacion26_NoPresente");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.distanceNm, exp["distanceNm"].toDouble());
    }

    // ── TBL_06 ───────────────────────────────────────────────────────────────
    void test_TBL_06_Estacion46_NoPresente() {
        QJsonObject tc  = findCase("TBL_06_Estacion46_NoPresente");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.distanceNm, exp["distanceNm"].toDouble());
    }

    // ── TBL_07 ───────────────────────────────────────────────────────────────
    void test_TBL_07_Estacion67_ValoresCorrectos() {
        QJsonObject tc  = findCase("TBL_07_Estacion67_ValoresCorrectos");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_08 ───────────────────────────────────────────────────────────────
    void test_TBL_08_StationCount_Es68() {
        QJsonObject tc  = findCase("TBL_08_StationCount_Es68");
        QJsonObject exp = tc["expected"].toObject();

        QCOMPARE(TwoWStationTable::stationCount(), exp["stationCount"].toInt());
    }

    // ── TBL_09 ───────────────────────────────────────────────────────────────
    void test_TBL_09_Estacion3_AlEste() {
        QJsonObject tc  = findCase("TBL_09_Estacion3_AlEste");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }

    // ── TBL_10 ───────────────────────────────────────────────────────────────
    void test_TBL_10_Estacion6_AlSurOeste() {
        QJsonObject tc  = findCase("TBL_10_Estacion6_AlSurOeste");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg,  exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm,  exp["distanceNm"].toDouble());
    }
};

QTEST_APPLESS_MAIN(TestTwoWStationTable)
#include "test_twowstationtable.moc"
