#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>
#include <QStringList>

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
        QVERIFY2(!m_cases.isEmpty(), "twowstationtable_cases.json must not be empty");

        const QStringList requiredIds = {
            QStringLiteral("TBL_01_Estacion1_ValoresCorrectos"),
            QStringLiteral("TBL_02_Estacion7_ValoresCorrectos"),
            QStringLiteral("TBL_03_Estacion12_ValoresCorrectos"),
            QStringLiteral("TBL_04_Estacion68_ValoresCorrectos"),
            QStringLiteral("TBL_05_Estacion26_NoPresente"),
            QStringLiteral("TBL_06_Estacion46_NoPresente"),
            QStringLiteral("TBL_07_Estacion67_ValoresCorrectos"),
            QStringLiteral("TBL_08_StationCount_Es68"),
            QStringLiteral("TBL_09_Estacion3_AlEste"),
            QStringLiteral("TBL_10_Estacion6_AlSurOeste")
        };

        QSet<QString> seenIds;
        for (const QJsonValue& value : m_cases) {
            QVERIFY2(value.isObject(), "Each station table case must be a JSON object");

            const QJsonObject tc = value.toObject();
            const QString id = tc["id"].toString();
            QVERIFY2(!id.isEmpty(), "Each station table case must define a non-empty id");
            QVERIFY2(!seenIds.contains(id),
                     qPrintable(QStringLiteral("Duplicated station table case id: %1").arg(id)));
            seenIds.insert(id);

            const QJsonObject exp = tc["expected"].toObject();
            QVERIFY2(!exp.isEmpty(),
                     qPrintable(QStringLiteral("Case %1 must define expected values").arg(id)));

            if (id == QStringLiteral("TBL_08_StationCount_Es68")) {
                QVERIFY2(exp["stationCount"].isDouble(),
                         qPrintable(QStringLiteral("Case %1 must define expected.stationCount").arg(id)));
                continue;
            }

            QVERIFY2(tc["stationNumber"].isDouble(),
                     qPrintable(QStringLiteral("Case %1 must define stationNumber").arg(id)));
            QVERIFY2(exp["azimuthDeg"].isDouble(),
                     qPrintable(QStringLiteral("Case %1 must define expected.azimuthDeg").arg(id)));
            QVERIFY2(exp["distanceNm"].isDouble(),
                     qPrintable(QStringLiteral("Case %1 must define expected.distanceNm").arg(id)));
        }

        for (const QString& id : requiredIds) {
            QVERIFY2(seenIds.contains(id),
                     qPrintable(QStringLiteral("Missing station table case id: %1").arg(id)));
        }
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

        QCOMPARE(entry.azimuthDeg, exp["azimuthDeg"].toDouble());
        QCOMPARE(entry.distanceNm, exp["distanceNm"].toDouble());
    }

    // ── TBL_06 ───────────────────────────────────────────────────────────────
    void test_TBL_06_Estacion46_NoPresente() {
        QJsonObject tc  = findCase("TBL_06_Estacion46_NoPresente");
        QJsonObject exp = tc["expected"].toObject();

        const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

        QCOMPARE(entry.azimuthDeg, exp["azimuthDeg"].toDouble());
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

    void test_TBL_NEW_01_CasosJsonComparanAzimutYDistancia() {
        int checkedCases = 0;

        for (const QJsonValue& value : m_cases) {
            const QJsonObject tc = value.toObject();
            if (!tc.contains("stationNumber"))
                continue;

            const QJsonObject exp = tc["expected"].toObject();
            const TwoWStationEntry& entry = TwoWStationTable::stationAt(tc["stationNumber"].toInt());

            QCOMPARE(entry.azimuthDeg, exp["azimuthDeg"].toDouble());
            QCOMPARE(entry.distanceNm, exp["distanceNm"].toDouble());
            ++checkedCases;
        }

        QVERIFY2(checkedCases > 0, "twowstationtable_cases.json must define station cases");
    }

    void test_TBL_NEW_02_InvariantesGlobales() {
        for (int stationNumber = 1; stationNumber <= TwoWStationTable::stationCount(); ++stationNumber) {
            const TwoWStationEntry& entry = TwoWStationTable::stationAt(stationNumber);
            const bool absent = entry.azimuthDeg == -1.0 && entry.distanceNm == -1.0;
            const bool present = entry.azimuthDeg >= 0.0
                                 && entry.azimuthDeg < 360.0
                                 && entry.distanceNm > 0.0;

            QVERIFY2(absent || present,
                     qPrintable(QStringLiteral("Station %1 must be valid or exactly (-1, -1)")
                                .arg(stationNumber)));
        }
    }

    void test_TBL_NEW_03_LimitesStationAt() {
        const int invalidStations[] = { -1, 0, 69 };

        for (int stationNumber : invalidStations) {
            const TwoWStationEntry& entry = TwoWStationTable::stationAt(stationNumber);
            QCOMPARE(entry.azimuthDeg, -1.0);
            QCOMPARE(entry.distanceNm, -1.0);
        }
    }
};

QTEST_APPLESS_MAIN(TestTwoWStationTable)
#include "test_twowstationtable.moc"
