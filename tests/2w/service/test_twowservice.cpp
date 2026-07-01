#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSet>
#include <QStringList>
#include <cmath>

#include "commandContext.h"
#include "TwoWService.h"
#include "entities/track.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Carga el JSON de recursos y devuelve el array de casos.
static QJsonArray loadCases()
{
    QFile file(":/json/json/twowservice_cases.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll())
               .object()["twoWServiceTestCases"]
               .toArray();
}

// Agrega un track al contexto con la posición y velocidad dadas.
static void addTrack(CommandContext& ctx, int id, double x, double y, double speed)
{
    Track t;
    t.setId(id);
    t.setX(static_cast<float>(x));
    t.setY(static_cast<float>(y));
    t.setVelocidadDmPerHour(speed);
    t.setCourseDeg(0.0);
    ctx.tracks.push_back(t);
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

static void verifyPointNear(const QPointF& actual, const QJsonObject& expected, double tolerance, const QString& label)
{
    verifyNear(actual.x(), expected["x"].toDouble(), tolerance, label + ".x");
    verifyNear(actual.y(), expected["y"].toDouble(), tolerance, label + ".y");
}

static QList<int> intListFromJson(const QJsonArray& values)
{
    QList<int> result;
    for (const QJsonValue& value : values) {
        result.append(value.toInt());
    }
    return result;
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

static TwoWSessionState makeInactiveSentinelState()
{
    TwoWSessionState state;
    state.active = false;
    state.guideTrackId = 77;
    state.bpStation = 12;
    state.circleRadiusNm = 9.5;
    seedCalculatedFields(state);
    return state;
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

// ─────────────────────────────────────────────────────────────────────────────
// Suite
// ─────────────────────────────────────────────────────────────────────────────
class TestTwoWService : public QObject
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
        QVERIFY2(!m_cases.isEmpty(), "twowservice_cases.json must not be empty");

        const QStringList requiredIds = {
            QStringLiteral("SVC_01_StartSession_ActivaLaSesion"),
            QStringLiteral("SVC_02_StopSession_DesactivaLaSesion"),
            QStringLiteral("SVC_03_Update_SinSesionActiva_NoCalcula"),
            QStringLiteral("SVC_04_Update_SinGuia_DetieneSesion"),
            QStringLiteral("SVC_05_Update_ConGuia_PopulaResultados"),
            QStringLiteral("SVC_06_Update_VelocidadCero_EtaInvalida"),
            QStringLiteral("SVC_07_StartSession_SobreescribeSesionAnterior")
        };

        const QStringList commonNumericInputs = {
            QStringLiteral("guideTrackId"),
            QStringLiteral("bpStation"),
            QStringLiteral("circleRadiusNm")
        };

        QSet<QString> seenIds;
        for (const QJsonValue& value : m_cases) {
            QVERIFY2(value.isObject(), "Each service case must be a JSON object");

            const QJsonObject tc = value.toObject();
            const QString id = tc["id"].toString();
            QVERIFY2(!id.isEmpty(), "Each service case must define a non-empty id");
            QVERIFY2(!seenIds.contains(id),
                     qPrintable(QStringLiteral("Duplicated service case id: %1").arg(id)));
            seenIds.insert(id);

            for (const QString& key : commonNumericInputs) {
                QVERIFY2(tc[key].isDouble(),
                         qPrintable(QStringLiteral("Case %1 must define numeric %2").arg(id, key)));
            }

            const QStringList optionalNumericInputs = {
                QStringLiteral("guideTrackX"),
                QStringLiteral("guideTrackY"),
                QStringLiteral("ownSpeed")
            };
            for (const QString& key : optionalNumericInputs) {
                if (tc.contains(key)) {
                    QVERIFY2(tc[key].isDouble(),
                             qPrintable(QStringLiteral("Case %1 must define numeric %2").arg(id, key)));
                }
            }

            if (tc.contains(QStringLiteral("selectedStations"))) {
                QVERIFY2(tc["selectedStations"].isArray(),
                         qPrintable(QStringLiteral("Case %1 must define selectedStations array").arg(id)));
            }

            const QJsonObject exp = tc["expected"].toObject();
            QVERIFY2(!exp.isEmpty(),
                     qPrintable(QStringLiteral("Case %1 must define expected values").arg(id)));
        }

        for (const QString& id : requiredIds) {
            QVERIFY2(seenIds.contains(id),
                     qPrintable(QStringLiteral("Missing service case id: %1").arg(id)));
        }
    }

    // ── SVC_01 ───────────────────────────────────────────────────────────────
    void test_SVC_01_StartSession_ActivaLaSesion() {
        QJsonObject tc  = findCase("SVC_01_StartSession_ActivaLaSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        TwoWService service(&ctx);

        service.startSession(
            tc["guideTrackId"].toInt(),
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );

        QCOMPARE(ctx.twoWSession.active,         exp["active"].toBool());
        QCOMPARE(ctx.twoWSession.guideTrackId,   exp["guideTrackId"].toInt());
        QCOMPARE(ctx.twoWSession.bpStation,      exp["bpStation"].toInt());
        QCOMPARE(ctx.twoWSession.circleRadiusNm, exp["circleRadiusNm"].toDouble());
    }

    // ── SVC_02 ───────────────────────────────────────────────────────────────
    void test_SVC_02_StopSession_DesactivaLaSesion() {
        QJsonObject tc  = findCase("SVC_02_StopSession_DesactivaLaSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        TwoWService service(&ctx);

        // Primero iniciamos para luego detener
        service.startSession(
            tc["guideTrackId"].toInt(),
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );
        QVERIFY(ctx.twoWSession.active);
        seedCalculatedFields(ctx.twoWSession);

        service.stopSession();

        QCOMPARE(ctx.twoWSession.active, exp["active"].toBool());
        QCOMPARE(ctx.twoWSession.guideTrackId, exp["guideTrackId"].toInt());
        QCOMPARE(ctx.twoWSession.bpStation, exp["bpStation"].toInt());
        if (exp["matchesDefaultState"].toBool()) {
            verifyDefaultSessionState(ctx.twoWSession);
        }
    }

    // ── SVC_03 ───────────────────────────────────────────────────────────────
    void test_SVC_03_Update_SinSesionActiva_NoCalcula() {
        QJsonObject tc = findCase("SVC_03_Update_SinSesionActiva_NoCalcula");

        CommandContext ctx;
        ctx.twoWSession = makeInactiveSentinelState();
        const TwoWSessionState before = ctx.twoWSession;

        // No llamamos startSession — sesión inactiva por defecto
        TwoWService service(&ctx);

        service.update();

        if (tc["expected"].toObject()["stateUnchanged"].toBool()) {
            verifySessionStateEquals(ctx.twoWSession, before);
        }
    }

    // ── SVC_04 ───────────────────────────────────────────────────────────────
    void test_SVC_04_Update_SinGuia_DetieneSesion() {
        QJsonObject tc  = findCase("SVC_04_Update_SinGuia_DetieneSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        TwoWService service(&ctx);

        // Iniciamos con un guía que NO existe en el contexto
        service.startSession(
            tc["guideTrackId"].toInt(),   // ID 9999 — no está en ctx.tracks
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );
        QVERIFY(ctx.twoWSession.active);
        seedCalculatedFields(ctx.twoWSession);

        service.update();

        // Al no encontrar el guía debe detener la sesión
        QCOMPARE(ctx.twoWSession.active, exp["active"].toBool());
        if (exp["matchesDefaultState"].toBool()) {
            verifyDefaultSessionState(ctx.twoWSession);
        }
    }

    // ── SVC_05 ───────────────────────────────────────────────────────────────
    void test_SVC_05_Update_ConGuia_PopulaResultados() {
        QJsonObject tc  = findCase("SVC_05_Update_ConGuia_PopulaResultados");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;

        // Agregar el track del Guía
        addTrack(ctx,
                 tc["guideTrackId"].toInt(),
                 tc["guideTrackX"].toDouble(),
                 tc["guideTrackY"].toDouble(),
                 5.0);

        // Agregar el track del Buque Propio (ID 0)
        addTrack(ctx, 0, 0.0, 0.0, tc["ownSpeed"].toDouble());

        TwoWService service(&ctx);
        service.startSession(
            tc["guideTrackId"].toInt(),
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );
        ctx.twoWSession.selectedStations = intListFromJson(tc["selectedStations"].toArray());

        service.update();

        const double tolerance = exp["tolerance"].toDouble();
        const double angleTolerance = exp["angleTolerance"].toDouble(tolerance);
        const double distanceTolerance = exp["distanceTolerance"].toDouble(tolerance);

        QCOMPARE(ctx.twoWSession.active, exp["active"].toBool());
        QCOMPARE(ctx.twoWSession.trackValid, exp["trackValid"].toBool());
        verifyPointNear(ctx.twoWSession.guideCircleCenter,
                        exp["guideCircleCenter"].toObject(),
                        tolerance,
                        "guideCircleCenter");
        verifyPointNear(ctx.twoWSession.ownCircleCenter,
                        exp["ownCircleCenter"].toObject(),
                        tolerance,
                        "ownCircleCenter");

        const QJsonArray expectedAllies = exp["allyCircleCenters"].toArray();
        QCOMPARE(ctx.twoWSession.allyCircleCenters.size(), expectedAllies.size());
        for (int i = 0; i < expectedAllies.size(); ++i) {
            verifyPointNear(ctx.twoWSession.allyCircleCenters[i],
                            expectedAllies[i].toObject(),
                            tolerance,
                            QStringLiteral("allyCircleCenters[%1]").arg(i));
        }

        QCOMPARE(ctx.twoWSession.etaValid, exp["etaValid"].toBool());
        verifyNear(ctx.twoWSession.courseToStationDeg,
                   exp["courseToStationDeg"].toDouble(),
                   angleTolerance,
                   "courseToStationDeg");
        verifyNear(ctx.twoWSession.timeToStationMin,
                   exp["timeToStationMin"].toDouble(),
                   tolerance,
                   "timeToStationMin");
        verifyNear(ctx.twoWSession.currentAzimuthDeg,
                   exp["currentAzimuthDeg"].toDouble(),
                   angleTolerance,
                   "currentAzimuthDeg");
        verifyNear(ctx.twoWSession.currentDistanceNm,
                   exp["currentDistanceNm"].toDouble(),
                   distanceTolerance,
                   "currentDistanceNm");
        verifyNear(ctx.twoWSession.expectedAzimuthDeg,
                   exp["expectedAzimuthDeg"].toDouble(),
                   angleTolerance,
                   "expectedAzimuthDeg");
        verifyNear(ctx.twoWSession.expectedDistanceNm,
                   exp["expectedDistanceNm"].toDouble(),
                   tolerance,
                   "expectedDistanceNm");
    }

    // ── SVC_06 ───────────────────────────────────────────────────────────────
    void test_SVC_06_Update_VelocidadCero_EtaInvalida() {
        QJsonObject tc  = findCase("SVC_06_Update_VelocidadCero_EtaInvalida");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;

        addTrack(ctx,
                 tc["guideTrackId"].toInt(),
                 tc["guideTrackX"].toDouble(),
                 tc["guideTrackY"].toDouble(),
                 5.0);

        // Buque propio con velocidad 0
        addTrack(ctx, 0, 0.0, 0.0, tc["ownSpeed"].toDouble());

        TwoWService service(&ctx);
        service.startSession(
            tc["guideTrackId"].toInt(),
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );

        service.update();

        QCOMPARE(ctx.twoWSession.etaValid, exp["etaValid"].toBool());
        QCOMPARE(ctx.twoWSession.timeToStationMin, 0.0);
    }

    // ── SVC_07 ───────────────────────────────────────────────────────────────
    void test_SVC_07_StartSession_SobreescribeSesionAnterior() {
        QJsonObject tc  = findCase("SVC_07_StartSession_SobreescribeSesionAnterior");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        TwoWService service(&ctx);

        // Primera sesión
        service.startSession(1, 12, 1.0);

        // Segunda sesión sobreescribe la anterior
        service.startSession(
            tc["guideTrackId"].toInt(),
            tc["bpStation"].toInt(),
            tc["circleRadiusNm"].toDouble()
        );

        QCOMPARE(ctx.twoWSession.active,         exp["active"].toBool());
        QCOMPARE(ctx.twoWSession.guideTrackId,   exp["guideTrackId"].toInt());
        QCOMPARE(ctx.twoWSession.bpStation,      exp["bpStation"].toInt());
        QCOMPARE(ctx.twoWSession.circleRadiusNm, exp["circleRadiusNm"].toDouble());
    }
};

QTEST_APPLESS_MAIN(TestTwoWService)
#include "test_twowservice.moc"
