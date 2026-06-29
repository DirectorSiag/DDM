#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "haService.h"
#include "commandContext.h"
#include "entities/track.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static QJsonArray loadCases()
{
    QFile file(":/json/json/haservice_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
               .object()["haServiceTestCases"].toArray();
}

// Inyecta el Track 0 (Buque Propio) en el contexto con posición (0,0)
static void injectTrack0(CommandContext& ctx, double course = 90.0, double speedKnots = 10.0)
{
    ctx.emplaceTrackFront(
        0,
        TrackData::SPC,
        TrackData::Pending,
        TrackData::Auto,
        0.0f, 0.0f,
        speedKnots,
        course,
        TrackData::SPC
    );
    ctx.ownShip.courseDeg  = course;
    ctx.ownShip.speedKnots = speedKnots;
    ctx.ownShip.valid      = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Suite
// ─────────────────────────────────────────────────────────────────────────────

class TestHaService : public QObject
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
        QVERIFY2(!m_cases.isEmpty(), "No se pudo cargar haservice_cases.json. Revisá el .qrc");
    }

    // ── SVC_01 ───────────────────────────────────────────────────────────────
    void test_SVC_01_StartAtCursor_ActivaSesion() {
        QJsonObject tc  = findCase("SVC_01_StartAtCursor_ActivaSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtCursor(
            tc["cursorX"].toDouble(), tc["cursorY"].toDouble());

        QVERIFY2(r.ok, qPrintable(r.message));
        QCOMPARE(ctx.haSession.active,     exp["active"].toBool());
        QCOMPARE(ctx.haSession.fallPointX, exp["fallPointX"].toDouble());
        QCOMPARE(ctx.haSession.fallPointY, exp["fallPointY"].toDouble());
    }

    // ── SVC_02 ───────────────────────────────────────────────────────────────
    void test_SVC_02_Stop_ReseteSesion() {
        QJsonObject tc  = findCase("SVC_02_Stop_ReseteSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        // Iniciamos primero para tener sesión activa
        service.startSessionAtCursor(5.0, 0.0);
        QVERIFY(ctx.haSession.active);

        const HaOperationResult r = service.stopSession();
        QVERIFY2(r.ok, qPrintable(r.message));
        QCOMPARE(ctx.haSession.active, exp["active"].toBool());
    }

    // ── SVC_03 ───────────────────────────────────────────────────────────────
    void test_SVC_03_Stop_SinSesionActiva_Falla() {
        QJsonObject tc  = findCase("SVC_03_Stop_SinSesionActiva_Falla");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        const HaOperationResult r = service.stopSession();
        QCOMPARE(r.ok, exp["ok"].toBool());
    }

    // ── SVC_04 ───────────────────────────────────────────────────────────────
    void test_SVC_04_StartAtBearing_RangoAzimutInvalido() {
        QJsonObject tc  = findCase("SVC_04_StartAtBearing_RangoAzimutInvalido");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        injectTrack0(ctx);
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtBearing(
            tc["azimuthDeg"].toDouble(), tc["distanceYards"].toDouble());
        QCOMPARE(r.ok, exp["ok"].toBool());
    }

    // ── SVC_05 ───────────────────────────────────────────────────────────────
    void test_SVC_05_StartAtBearing_DistanciaInvalida() {
        QJsonObject tc  = findCase("SVC_05_StartAtBearing_DistanciaInvalida");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        injectTrack0(ctx);
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtBearing(
            tc["azimuthDeg"].toDouble(), tc["distanceYards"].toDouble());
        QCOMPARE(r.ok, exp["ok"].toBool());
    }

    // ── SVC_06 ───────────────────────────────────────────────────────────────
    void test_SVC_06_StartAtCursor_SobreescribeSesionPrevia() {
        QJsonObject tc  = findCase("SVC_06_StartAtCursor_SobreescribeSesionPrevia");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        // Primera sesión
        service.startSessionAtCursor(1.0, 1.0);
        QVERIFY(ctx.haSession.active);

        // Segunda sesión — sobreescribe
        const HaOperationResult r = service.startSessionAtCursor(
            tc["cursorX"].toDouble(), tc["cursorY"].toDouble());

        QVERIFY2(r.ok, qPrintable(r.message));
        QCOMPARE(ctx.haSession.active,     exp["active"].toBool());
        QCOMPARE(ctx.haSession.fallPointX, exp["fallPointX"].toDouble());
        QCOMPARE(ctx.haSession.fallPointY, exp["fallPointY"].toDouble());
    }

    // ── SVC_07 ───────────────────────────────────────────────────────────────
    void test_SVC_07_LatLon_SinGeoposicion_Falla() {
        QJsonObject tc  = findCase("SVC_07_LatLon_SinGeoposicion_Falla");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        // No seteamos geo-posición en ownShip → bpHasGeo será false
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtLatLonDms(
            tc["latDeg"].toInt(), tc["latMin"].toInt(), tc["latSec"].toDouble(),
            tc["lonDeg"].toInt(), tc["lonMin"].toInt(), tc["lonSec"].toDouble());

        QCOMPARE(r.ok, exp["ok"].toBool());
    }
};

QTEST_APPLESS_MAIN(TestHaService)
#include "test_haservice.moc"
