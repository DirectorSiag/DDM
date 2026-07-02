#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <cmath>

#include "TwoWCommand.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"

static QJsonArray loadCases()
{
    QFile file(":/json/json/twowcommand_cases.json");
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll())
        .object()["twoWCommandTestCases"]
        .toArray();
}

static bool parseCommandLine(const QString& commandLine, CommandInvocation& inv, QString& parseError)
{
    CommandParser parser;
    return parser.parse(commandLine, inv, parseError);
}

static void verifyNear(double actual, double expected, double tolerance, const QString& label)
{
    QVERIFY2(std::abs(actual - expected) <= tolerance,
             qPrintable(QString("%1=%2 no coincide con esperado=%3 tolerancia=%4")
                        .arg(label).arg(actual).arg(expected).arg(tolerance)));
}

static void verifyPointNear(const QPointF& actual, const QPointF& expected, double tolerance, const QString& label)
{
    verifyNear(actual.x(), expected.x(), tolerance, label + ".x");
    verifyNear(actual.y(), expected.y(), tolerance, label + ".y");
}

static void verifyIntListEquals(const QList<int>& actual, const QList<int>& expected, const QString& label)
{
    QCOMPARE(actual.size(), expected.size());
    for (int i = 0; i < actual.size(); ++i) {
        QCOMPARE(actual[i], expected[i]);
    }
    Q_UNUSED(label)
}

static void verifyPointListNear(const QList<QPointF>& actual,
                                const QList<QPointF>& expected,
                                double tolerance,
                                const QString& label)
{
    QCOMPARE(actual.size(), expected.size());
    for (int i = 0; i < actual.size(); ++i) {
        verifyPointNear(actual[i], expected[i], tolerance, QString("%1[%2]").arg(label).arg(i));
    }
}

static void seedCalculatedFields(TwoWSessionState& state)
{
    state.selectedStations = QList<int>{1, 2, 3};
    state.trackValid = true;
    state.guideCircleCenter = QPointF(11.0, 22.0);
    state.ownCircleCenter = QPointF(33.0, 44.0);
    state.allyCircleCenters = QList<QPointF>{QPointF(55.0, 66.0), QPointF(77.0, 88.0)};
    state.etaValid = true;
    state.courseToStationDeg = 123.0;
    state.timeToStationMin = 456.0;
    state.currentAzimuthDeg = 78.0;
    state.currentDistanceNm = 9.0;
    state.expectedAzimuthDeg = 270.0;
    state.expectedDistanceNm = 4.0;
}

static TwoWSessionState makeActiveSentinelState()
{
    TwoWSessionState state;
    state.active = true;
    state.guideTrackId = 10;
    state.bpStation = 5;
    state.circleRadiusNm = 2.5;
    seedCalculatedFields(state);
    return state;
}

static TwoWSessionState makeInactiveSentinelState()
{
    TwoWSessionState state = makeActiveSentinelState();
    state.active = false;
    state.guideTrackId = 77;
    state.bpStation = 12;
    state.circleRadiusNm = 9.5;
    return state;
}

static void applyPrecondition(CommandContext& ctx, const QString& precondition)
{
    if (precondition == QStringLiteral("activeSentinel")) {
        ctx.twoWSession = makeActiveSentinelState();
    } else if (precondition == QStringLiteral("inactiveSentinel")) {
        ctx.twoWSession = makeInactiveSentinelState();
    }
}

static void verifySessionStateEquals(const TwoWSessionState& actual, const TwoWSessionState& expected)
{
    QCOMPARE(actual.active, expected.active);
    QCOMPARE(actual.guideTrackId, expected.guideTrackId);
    QCOMPARE(actual.bpStation, expected.bpStation);
    verifyNear(actual.circleRadiusNm, expected.circleRadiusNm, 0.0, "circleRadiusNm");
    verifyIntListEquals(actual.selectedStations, expected.selectedStations, "selectedStations");
    QCOMPARE(actual.trackValid, expected.trackValid);
    verifyPointNear(actual.guideCircleCenter, expected.guideCircleCenter, 0.0, "guideCircleCenter");
    verifyPointNear(actual.ownCircleCenter, expected.ownCircleCenter, 0.0, "ownCircleCenter");
    verifyPointListNear(actual.allyCircleCenters, expected.allyCircleCenters, 0.0, "allyCircleCenters");
    QCOMPARE(actual.etaValid, expected.etaValid);
    verifyNear(actual.courseToStationDeg, expected.courseToStationDeg, 0.0, "courseToStationDeg");
    verifyNear(actual.timeToStationMin, expected.timeToStationMin, 0.0, "timeToStationMin");
    verifyNear(actual.currentAzimuthDeg, expected.currentAzimuthDeg, 0.0, "currentAzimuthDeg");
    verifyNear(actual.currentDistanceNm, expected.currentDistanceNm, 0.0, "currentDistanceNm");
    verifyNear(actual.expectedAzimuthDeg, expected.expectedAzimuthDeg, 0.0, "expectedAzimuthDeg");
    verifyNear(actual.expectedDistanceNm, expected.expectedDistanceNm, 0.0, "expectedDistanceNm");
}

static void verifyDefaultSessionState(const TwoWSessionState& actual)
{
    verifySessionStateEquals(actual, TwoWSessionState{});
}

static void verifySessionMatchesExpected(const TwoWSessionState& actual, const QJsonObject& expected)
{
    if (expected.contains(QStringLiteral("active"))) {
        QCOMPARE(actual.active, expected["active"].toBool());
    }
    if (expected.contains(QStringLiteral("guideTrackId"))) {
        QCOMPARE(actual.guideTrackId, expected["guideTrackId"].toInt());
    }
    if (expected.contains(QStringLiteral("bpStation"))) {
        QCOMPARE(actual.bpStation, expected["bpStation"].toInt());
    }
    if (expected.contains(QStringLiteral("circleRadiusNm"))) {
        verifyNear(actual.circleRadiusNm, expected["circleRadiusNm"].toDouble(), 0.0, "circleRadiusNm");
    }
}

static void verifyTextContains(const QString& actual, const QString& expectedText)
{
    const bool containsMsg = actual.contains(expectedText, Qt::CaseInsensitive);
    if (!containsMsg) {
        qDebug() << "\nTEXTO ESPERADO:" << expectedText;
        qDebug() << "TEXTO OBTENIDO:" << actual << "\n";
    }
    QVERIFY2(containsMsg, "El texto obtenido no contiene el fragmento esperado.");
}

class TestTwoWCommand : public QObject
{
    Q_OBJECT

private:
    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const
    {
        for (const QJsonValue& value : m_cases) {
            const QJsonObject tc = value.toObject();
            if (tc["id"].toString() == id) {
                return tc;
            }
        }
        return {};
    }

private slots:
    void initTestCase()
    {
        m_cases = loadCases();
        QVERIFY2(!m_cases.isEmpty(), "twowcommand_cases.json must not be empty");

        const QStringList requiredIds = {
            QStringLiteral("CMD_01_Camino_Exitoso_Iniciacion_Valida"),
            QStringLiteral("CMD_02_Error_Estacion_Fuera_De_Rango_Maximo"),
            QStringLiteral("CMD_03_Error_Radio_Invalido_Negativo"),
            QStringLiteral("CMD_04_Error_Estacion_Fuera_De_Rango_Minimo"),
            QStringLiteral("CMD_05_Error_Falta_Bp"),
            QStringLiteral("CMD_06_Camino_Exitoso_Detener_Formacion"),
            QStringLiteral("CMD_07_Error_Estacion_No_Presente"),
            QStringLiteral("CMD_NEW_01_Info_SinSesionActiva_NoModificaEstado"),
            QStringLiteral("CMD_NEW_02_Info_ConSesionActiva_NoModificaEstado"),
            QStringLiteral("CMD_NEW_03_RadioOmitido_UsaValorPorDefecto"),
            QStringLiteral("CMD_NEW_04_GuiaNoEntera_RechazaSinCambios"),
            QStringLiteral("CMD_NEW_04_BpNoEntero_RechazaSinCambios"),
            QStringLiteral("CMD_NEW_04_RadioCero_RechazaSinCambios"),
            QStringLiteral("CMD_NEW_04_BpFlagSinValor_RechazaSinCambios"),
            QStringLiteral("CMD_NEW_04_TokenSinFormato_RechazaSinCambios"),
            QStringLiteral("CMD_NEW_06_Mayusculas_Normalizadas")
        };

        const QSet<QString> allowedPreconditions = {
            QString(),
            QStringLiteral("activeSentinel"),
            QStringLiteral("inactiveSentinel")
        };

        QSet<QString> seenIds;
        for (const QJsonValue& value : m_cases) {
            QVERIFY2(value.isObject(), "Each command case must be a JSON object");

            const QJsonObject tc = value.toObject();
            const QString id = tc["id"].toString();
            QVERIFY2(!id.isEmpty(), "Each command case must define a non-empty id");
            QVERIFY2(!seenIds.contains(id),
                     qPrintable(QStringLiteral("Duplicated command case id: %1").arg(id)));
            seenIds.insert(id);

            QVERIFY2(tc["commandLine"].isString(),
                     qPrintable(QStringLiteral("Case %1 must define commandLine").arg(id)));
            QVERIFY2(tc["expectedSuccess"].isBool(),
                     qPrintable(QStringLiteral("Case %1 must define expectedSuccess").arg(id)));
            QVERIFY2(tc["expectedMessageContains"].isString(),
                     qPrintable(QStringLiteral("Case %1 must define expectedMessageContains").arg(id)));
            QVERIFY2(tc["expected"].isObject(),
                     qPrintable(QStringLiteral("Case %1 must define expected").arg(id)));
            if (tc.contains(QStringLiteral("expectedMessageContainsAll"))) {
                QVERIFY2(tc["expectedMessageContainsAll"].isArray(),
                         qPrintable(QStringLiteral("Case %1 expectedMessageContainsAll must be an array").arg(id)));
                const QJsonArray expectedMessages = tc["expectedMessageContainsAll"].toArray();
                for (const QJsonValue& expectedMessage : expectedMessages) {
                    QVERIFY2(expectedMessage.isString(),
                             qPrintable(QStringLiteral("Case %1 expectedMessageContainsAll entries must be strings").arg(id)));
                }
            }

            const QString precondition = tc["precondition"].toString();
            QVERIFY2(allowedPreconditions.contains(precondition),
                     qPrintable(QStringLiteral("Case %1 has invalid precondition %2").arg(id, precondition)));

            const QJsonObject expected = tc["expected"].toObject();
            if (expected.contains(QStringLiteral("active"))) {
                QVERIFY2(expected["active"].isBool(),
                         qPrintable(QStringLiteral("Case %1 expected.active must be bool").arg(id)));
            }
            for (const QString& key : {QStringLiteral("guideTrackId"), QStringLiteral("bpStation")}) {
                if (expected.contains(key)) {
                    QVERIFY2(expected[key].isDouble(),
                             qPrintable(QStringLiteral("Case %1 expected.%2 must be numeric").arg(id, key)));
                }
            }
            if (expected.contains(QStringLiteral("circleRadiusNm"))) {
                QVERIFY2(expected["circleRadiusNm"].isDouble(),
                         qPrintable(QStringLiteral("Case %1 expected.circleRadiusNm must be numeric").arg(id)));
            }
        }

        for (const QString& id : requiredIds) {
            QVERIFY2(seenIds.contains(id),
                     qPrintable(QStringLiteral("Missing command case id: %1").arg(id)));
        }
    }

    void test_ComandosJson_data()
    {
        QTest::addColumn<QString>("id");

        const QJsonArray cases = loadCases();
        QVERIFY2(!cases.isEmpty(), "twowcommand_cases.json must not be empty");

        for (const QJsonValue& value : cases) {
            const QJsonObject tc = value.toObject();
            const QString id = tc["id"].toString();
            QTest::newRow(qPrintable(id)) << id;
        }
    }

    void test_ComandosJson()
    {
        QFETCH(QString, id);

        const QJsonObject tc = findCase(id);
        QVERIFY2(!tc.isEmpty(), qPrintable(QStringLiteral("Missing command case id: %1").arg(id)));

        const QString commandLine = tc["commandLine"].toString();
        const QString precondition = tc["precondition"].toString();
        const bool expectedSuccess = tc["expectedSuccess"].toBool();
        const QString expectedMessageContains = tc["expectedMessageContains"].toString();
        const QJsonObject expected = tc["expected"].toObject();

        CommandInvocation inv;
        QString parseError;
        QVERIFY2(parseCommandLine(commandLine, inv, parseError),
                 qPrintable(QStringLiteral("El CommandParser fallo leyendo %1: %2").arg(id, parseError)));

        CommandContext ctx;
        applyPrecondition(ctx, precondition);
        const TwoWSessionState before = ctx.twoWSession;

        TwoWCommand cmd;
        const CommandResult res = cmd.execute(inv, ctx);

        QCOMPARE(res.ok, expectedSuccess);
        verifyTextContains(res.message, expectedMessageContains);

        const QJsonArray expectedMessages = tc["expectedMessageContainsAll"].toArray();
        for (const QJsonValue& expectedMessage : expectedMessages) {
            verifyTextContains(res.message, expectedMessage.toString());
        }

        if (expected["stateUnchanged"].toBool()) {
            verifySessionStateEquals(ctx.twoWSession, before);
        }
        if (expected["matchesDefaultState"].toBool()) {
            verifyDefaultSessionState(ctx.twoWSession);
        }

        verifySessionMatchesExpected(ctx.twoWSession, expected);
    }
};

QTEST_APPLESS_MAIN(TestTwoWCommand)
#include "test_twowcommand.moc"
