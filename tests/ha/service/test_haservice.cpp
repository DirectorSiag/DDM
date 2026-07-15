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

// Inyecta el Track 0 (Buque Propio) en el contexto. Se agregaron X e Y para los nuevos tests.
static void injectTrack0(CommandContext& ctx, double x = 0.0, double y = 0.0, double course = 90.0, double speedKnots = 10.0)
{
    ctx.emplaceTrackFront(
        0,
        TrackData::SPC,
        TrackData::Pending,
        TrackData::Auto,
        x, y,
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
        QCOMPARE(ctx.haSessions[0].active,     exp["active"].toBool());
        QCOMPARE(ctx.haSessions[0].fallPointX, exp["fallPointX"].toDouble());
        QCOMPARE(ctx.haSessions[0].fallPointY, exp["fallPointY"].toDouble());
    }

    // ── SVC_02 ───────────────────────────────────────────────────────────────
    void test_SVC_02_Stop_ReseteSesion() {
        QJsonObject tc  = findCase("SVC_02_Stop_ReseteSesion");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        service.startSessionAtCursor(5.0, 0.0);
        QVERIFY(ctx.haSessions[0].active);

        const HaOperationResult r = service.stopSession(-1); // -1 borra todos
        QVERIFY2(r.ok, qPrintable(r.message));
        QCOMPARE(ctx.haSessions[0].active, exp["active"].toBool());
    }

    // ── SVC_03 ───────────────────────────────────────────────────────────────
    void test_SVC_03_Stop_SinSesionActiva_Falla() {
        QJsonObject tc  = findCase("SVC_03_Stop_SinSesionActiva_Falla");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        const HaOperationResult r = service.stopSession(-1);
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

    // ── SVC_06 (Verifica el slot 2) ───────────────────────────────
    void test_SVC_06_StartAtCursor_AsignaNuevoSlot() {
        QJsonObject tc  = findCase("SVC_06_StartAtCursor_AsignaNuevoSlot");
        QJsonObject exp = tc["expected"].toObject();
        QJsonArray expectedSessions = exp["haSessions"].toArray();
        QJsonObject expSlot2 = expectedSessions[1].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        // Primera sesión ocupa el slot 1 (índice 0)
        service.startSessionAtCursor(1.0, 1.0);
        QVERIFY(ctx.haSessions[0].active);

        // Segunda sesión — ocupa el slot 2 (índice 1) sin pisar la primera
        const HaOperationResult r = service.startSessionAtCursor(
            tc["cursorX"].toDouble(), tc["cursorY"].toDouble());

        QVERIFY2(r.ok, qPrintable(r.message));

        // Verificamos que el slot 1 sigue intacto
        QVERIFY(ctx.haSessions[0].active);
        QCOMPARE(ctx.haSessions[0].fallPointX, 1.0);
        QCOMPARE(ctx.haSessions[0].fallPointY, 1.0);

        // Verificamos que el slot 2 tiene los datos nuevos del JSON
        QVERIFY(ctx.haSessions[1].active);
        QCOMPARE(ctx.haSessions[1].slotIndex, expSlot2["slotIndex"].toInt());
        QCOMPARE(ctx.haSessions[1].fallPointX, expSlot2["fallPointX"].toDouble());
        QCOMPARE(ctx.haSessions[1].fallPointY, expSlot2["fallPointY"].toDouble());

        // Verificamos que se auto-seleccionó el slot 2
        QCOMPARE(ctx.activeHaSlot, exp["activeHaSlot"].toInt());
    }

    // ── SVC_07 ───────────────────────────────────────────────────────────────
    void test_SVC_07_StartAtOwnShip() {
        QJsonObject tc  = findCase("SVC_07_StartAtOwnShip");
        QJsonObject setup = tc["setup"].toObject();
        QJsonObject exp = tc["expected"].toObject();
        QJsonObject expSlot1 = exp["haSessions"].toArray()[0].toObject();

        CommandContext ctx;
        // Inyectamos el BP en la coordenada específica del JSON
        injectTrack0(ctx, setup["ownShipX"].toDouble(), setup["ownShipY"].toDouble());
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtOwnShip();

        QVERIFY2(r.ok, qPrintable(r.message));
        QVERIFY(ctx.haSessions[0].active);
        QCOMPARE(ctx.haSessions[0].fallPointX, expSlot1["fallPointX"].toDouble());
        QCOMPARE(ctx.haSessions[0].fallPointY, expSlot1["fallPointY"].toDouble());
    }

    // ── SVC_08 ───────────────────────────────────────────────────────────────
    void test_SVC_08_StartAtBearing() {
        QJsonObject tc  = findCase("SVC_08_StartAtBearing");
        QJsonObject setup = tc["setup"].toObject();
        QJsonObject exp = tc["expected"].toObject();
        QJsonObject expSlot1 = exp["haSessions"].toArray()[0].toObject();

        CommandContext ctx;
        injectTrack0(ctx, setup["ownShipX"].toDouble(), setup["ownShipY"].toDouble());
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtBearing(
            tc["azimuthDeg"].toDouble(), tc["distanceYards"].toDouble());

        QVERIFY2(r.ok, qPrintable(r.message));
        QVERIFY(ctx.haSessions[0].active);
        // Usamos qFuzzyCompare o qRound si hay errores de punto flotante por trigonometría
        QCOMPARE(qRound(ctx.haSessions[0].fallPointX), qRound(expSlot1["fallPointX"].toDouble()));
        QCOMPARE(qRound(ctx.haSessions[0].fallPointY), qRound(expSlot1["fallPointY"].toDouble()));
    }

    // ── SVC_09 ───────────────────────────────────────────────────────
    void test_SVC_09_LatLon_SinGeoposicion_Falla() {
        QJsonObject tc  = findCase("SVC_09_LatLon_SinGeoposicion_Falla");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        const HaOperationResult r = service.startSessionAtLatLonDms(
            tc["latDeg"].toInt(), tc["latMin"].toInt(), tc["latSec"].toDouble(),
            tc["lonDeg"].toInt(), tc["lonMin"].toInt(), tc["lonSec"].toDouble());

        QCOMPARE(r.ok, exp["ok"].toBool());
    }

    // ── SVC_10 ─────────────────────────────────────────────────────────
    void test_SVC_10_MaxSlotsLimit() {
        QJsonObject tc  = findCase("SVC_10_MaxSlotsLimit");
        QJsonObject exp = tc["expected"].toObject();

        CommandContext ctx;
        HaService service(&ctx);

        // 1. Llenamos los 10 slots permitidos
        for(int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
            service.startSessionAtCursor(double(i), double(i));
            QVERIFY(ctx.haSessions[i].active);
        }

        // 2. Intentamos crear la emergencia nro 11 leyendo las coords del JSON
        service.startSessionAtCursor(
            tc["overflowCursorX"].toDouble(),
            tc["overflowCursorY"].toDouble()
            );

        // 3. Verificamos que NO pisó el primer slot (debería seguir en 0.0)
        QCOMPARE(ctx.haSessions[0].fallPointX, exp["firstSlotOriginalX"].toDouble());

        // 4. Verificamos que todos siguen activos y el conteo es exactamente 10
        int activeCount = 0;
        for(int i = 0; i < CommandContext::kMaxHaSessions; ++i) {
            if (ctx.haSessions[i].active) activeCount++;
        }
        QCOMPARE(activeCount, exp["activeCount"].toInt());
    }
};

QTEST_APPLESS_MAIN(TestHaService)
#include "test_haservice.moc"