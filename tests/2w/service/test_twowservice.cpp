#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

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
        QVERIFY2(!m_cases.isEmpty(),
                 "No se pudo cargar twowservice_cases.json. Revisá el .qrc");
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

        service.stopSession();

        QCOMPARE(ctx.twoWSession.active,       exp["active"].toBool());
        QCOMPARE(ctx.twoWSession.guideTrackId, exp["guideTrackId"].toInt());
        QCOMPARE(ctx.twoWSession.bpStation,    exp["bpStation"].toInt());
    }

    // ── SVC_03 ───────────────────────────────────────────────────────────────
    void test_SVC_03_Update_SinSesionActiva_NoCalcula() {
        QJsonObject tc = findCase("SVC_03_Update_SinSesionActiva_NoCalcula");

        CommandContext ctx;
        // No llamamos startSession — sesión inactiva por defecto
        TwoWService service(&ctx);

        service.update();

        // twoWSession no debe haber cambiado
        QCOMPARE(ctx.twoWSession.active,        false);
        QCOMPARE(ctx.twoWSession.etaValid, false);
        QCOMPARE(ctx.twoWSession.courseToStationDeg, 0.0);
        QCOMPARE(ctx.twoWSession.timeToStationMin,   0.0);
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

        service.update();

        // Al no encontrar el guía debe detener la sesión
        QCOMPARE(ctx.twoWSession.active, exp["active"].toBool());
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

        service.update();

        // El guideCircleCenter debe coincidir con la posición del track guía
        if (exp["guideCenterMatchesTrack"].toBool()) {
            QCOMPARE(ctx.twoWSession.guideCircleCenter.x(),
                     tc["guideTrackX"].toDouble());
            QCOMPARE(ctx.twoWSession.guideCircleCenter.y(),
                     tc["guideTrackY"].toDouble());
        }

        // El ownCircleCenter no debe ser (0,0) — debe haberse proyectado
        if (exp["ownCenterNotZero"].toBool()) {
            bool notZero = ctx.twoWSession.ownCircleCenter.x() != 0.0 ||
                           ctx.twoWSession.ownCircleCenter.y() != 0.0;
            QVERIFY2(notZero, "ownCircleCenter sigue en (0,0) — el calculador no corrió");
        }
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
