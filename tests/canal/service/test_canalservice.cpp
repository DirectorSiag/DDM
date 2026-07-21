#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "canalService.h"
#include "commandContext.h"
#include "model/entities/track.h"

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/canalservice_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["canalServiceTestCases"].toArray();
}

// ─────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────
static CanalConfig configFromJson(const QJsonObject& in)
{
    CanalConfig cfg;
    cfg.setA = in["setA"].toBool(false); cfg.trackA = in["trackA"].toInt(-1);
    cfg.setB = in["setB"].toBool(false); cfg.trackB = in["trackB"].toInt(-1);
    cfg.setC = in["setC"].toBool(false); cfg.trackC = in["trackC"].toInt(-1);
    cfg.setD = in["setD"].toBool(false); cfg.trackD = in["trackD"].toInt(-1);
    return cfg;
}

static void setupContext(CommandContext& ctx, const QJsonObject& in)
{
    for (const QJsonValue& v : in["tracks"].toArray()) {
        QJsonObject t = v.toObject();
        Track track;
        track.setId(t["id"].toInt());
        track.setX(static_cast<float>(t["x"].toDouble()));
        track.setY(static_cast<float>(t["y"].toDouble()));
        ctx.tracks.push_back(track);
    }
}

class TestCanalService : public QObject {
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

    // ── startSession (data-driven) ──────────────────────────────

    void test_CANSVC_01_StartSession_ColumnaA_Exitosa()      { runStartSessionTest("CANSVC_01_StartSession_ColumnaA_Exitosa"); }
    void test_CANSVC_02_StartSession_MultiplesColumnas()     { runStartSessionTest("CANSVC_02_StartSession_MultiplesColumnas"); }
    void test_CANSVC_03_Error_TrackInexistente_ColumnaB()    { runStartSessionTest("CANSVC_03_Error_TrackInexistente_ColumnaB"); }
    void test_CANSVC_05_StartSession_NingunaColumnaSeteada() { runStartSessionTest("CANSVC_05_StartSession_NingunaColumnaSeteada"); }

    // ── startSession: casos con verificacion de estado interno ──

    void test_CANSVC_04_Error_ColumnaEnUso() {
        // ARRANGE: columna A ya activa en el contexto
        CommandContext ctx;
        Track t; t.setId(5); t.setX(0.0f); t.setY(5.0f);
        ctx.tracks.push_back(t);
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CanalConfig cfg;
        cfg.setA = true; cfg.trackA = 5;

        CanalService service(&ctx);

        // ACT
        CanalOperationResult res = service.startSession(cfg);

        // ASSERT
        QVERIFY(!res.success);
        QVERIFY2(res.message.contains("en uso", Qt::CaseInsensitive),
                 qPrintable(QString("Mensaje obtenido: %1").arg(res.message)));
    }

    void test_CANSVC_04b_TrackDuplicado_SinValidacionCruzada() {
        // NOTA: no hay validacion que impida usar el mismo track en dos columnas.
        // Este test documenta el comportamiento actual (no es necesariamente lo deseado).
        CommandContext ctx;
        Track t; t.setId(5); t.setX(0.0f); t.setY(5.0f);
        ctx.tracks.push_back(t);

        CanalConfig cfg;
        cfg.setA = true; cfg.trackA = 5;
        cfg.setB = true; cfg.trackB = 5; // mismo track que A

        CanalService service(&ctx);

        CanalOperationResult res = service.startSession(cfg);

        QVERIFY2(res.success, "startSession no rechaza tracks duplicados entre columnas (comportamiento actual)");
        QCOMPARE(ctx.canalSession.columnas[0].trackId, 5);
        QCOMPARE(ctx.canalSession.columnas[1].trackId, 5);
    }

    // ── stopSession (data-driven) ────────────────────────────────

    void test_CANSVC_06_StopSession_ConSesionActiva() {
        QJsonObject tc = findCase("CANSVC_06_StopSession_ConSesionActiva");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CanalService service(&ctx);
        CanalOperationResult res = service.stopSession();

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("Esperado: '%1' | Obtenido: '%2'").arg(ex["messageContains"].toString(), res.message)));
        QVERIFY(!ctx.canalSession.active);
        QVERIFY(!ctx.canalSession.columnas[0].active);
    }

    void test_CANSVC_07_StopSession_SinSesionActiva() {
        QJsonObject tc = findCase("CANSVC_07_StopSession_SinSesionActiva");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        CanalService service(&ctx);
        CanalOperationResult res = service.stopSession();

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("Esperado: '%1' | Obtenido: '%2'").arg(ex["messageContains"].toString(), res.message)));
    }

    // ── update() ─────────────────────────────────────────────────

    void test_CANSVC_08_Update_SesionInactiva_NoOp() {
        // ARRANGE: sesion global inactiva
        CommandContext ctx;
        CanalService service(&ctx);

        // ACT
        service.update();

        // ASSERT: nada cambia, no crashea
        QVERIFY(!ctx.canalSession.active);
        QVERIFY(!ctx.canalSession.columnas[0].active);
    }

    void test_CANSVC_09_Update_SinBuqueOwnTrack() {
        // ARRANGE: sesion activa con columna A activa, pero NO hay track id=0 (buque propio)
        CommandContext ctx;
        Track buoy; buoy.setId(1); buoy.setX(0.0f); buoy.setY(5.0f);
        ctx.tracks.push_back(buoy); // solo la boya, sin el track 0

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CanalService service(&ctx);

        // ACT
        service.update();

        // ASSERT: update() debe retornar sin tocar nada (no crashea, no calcula)
        QVERIFY(ctx.canalSession.active);
        QVERIFY(ctx.canalSession.columnas[0].active);
        QCOMPARE(ctx.canalSession.columnas[0].distanciaYardas, 0.0);
    }

    void test_CANSVC_10_Update_ColumnaConTrackBorrado_ResetInmediato() {
        // ARRANGE: columna A activa apuntando a un track que YA NO existe en el contexto
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f);
        ctx.tracks.push_back(own); // solo el buque propio, la boya (trackId=99) no existe

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 99;

        CanalService service(&ctx);

        // ACT
        service.update();

        // ASSERT: la columna se resetea Y la sesion global se apaga EN EL MISMO TICK
        // (a diferencia del reset por 'hasTriggeredAlarm && isBehind', que tarda un ciclo mas)
        QVERIFY2(!ctx.canalSession.columnas[0].active, "La columna debe resetearse al no encontrar el track de la boya");
        QVERIFY2(!ctx.canalSession.active, "La sesion global debe apagarse en el mismo tick si era la unica columna activa");
    }

    void test_CANSVC_11_Update_CalculaCinematicaCorrecta() {
        // ARRANGE: boya al norte (az=0), BP en el origen, mismo rumbo
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f); own.setCourseDeg(0.0);
        ctx.tracks.push_back(own);
        Track buoy; buoy.setId(1); buoy.setX(0.0f); buoy.setY(5.0f);
        ctx.tracks.push_back(buoy);

        ctx.ownShip.speedKnots = 9.8747; // → ownSpeedDm = 9.8747 / kDmToNm(0.98747) = 10.0

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CanalService service(&ctx);

        // ACT
        service.update();

        // ASSERT
        const auto& slot = ctx.canalSession.columnas[0];
        QVERIFY2(qAbs(slot.distanciaYardas - 10000.0) < 1.0,
                 qPrintable(QString("distanciaYardas: esperado ~10000, obtenido %1").arg(slot.distanciaYardas)));
        QVERIFY2(qAbs(slot.azimutVerdadero - 0.0) < 1.0,
                 qPrintable(QString("azimutVerdadero: esperado ~0, obtenido %1").arg(slot.azimutVerdadero)));
        QVERIFY2(qAbs(slot.timeToArrivalMin - 30.0) < 1.0,
                 qPrintable(QString("timeToArrivalMin: esperado ~30, obtenido %1").arg(slot.timeToArrivalMin)));
        QVERIFY(slot.etaValid);
        QVERIFY2(!slot.isAlarmActive, "No deberia estar en ventana de alarma (marcacion=0)");
        QVERIFY2(!slot.hasTriggeredAlarm, "La alarma nunca se disparo en este escenario");
    }

    void test_CANSVC_12_Update_ActivaAlarmaEnVentana() {
        // ARRANGE: boya al este (az=90 = marcacionRelativa=90, dentro de la ventana [85,95])
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f); own.setCourseDeg(0.0);
        ctx.tracks.push_back(own);
        Track buoy; buoy.setId(1); buoy.setX(5.0f); buoy.setY(0.0f);
        ctx.tracks.push_back(buoy);

        ctx.ownShip.speedKnots = 9.8747;

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CanalService service(&ctx);
        service.update();

        const auto& slot = ctx.canalSession.columnas[0];
        QVERIFY2(slot.isAlarmActive, "Deberia activarse la alarma: marcacionRelativa=90 esta en la ventana [85,95]");
        QVERIFY2(slot.hasTriggeredAlarm, "hasTriggeredAlarm debe quedar en true tras entrar en la ventana");
    }

    void test_CANSVC_13_Update_ResetAlSuperarTraves_SesionSigueActivaEsteTick() {
        // ARRANGE: la alarma YA se disparo en un ciclo anterior (hasTriggeredAlarm=true preseteado),
        // y ahora la boya esta en la zona 'detras del traves' (marcacionRelativa=180, isBehind=true)
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f); own.setCourseDeg(0.0);
        ctx.tracks.push_back(own);
        Track buoy; buoy.setId(1); buoy.setX(0.0f); buoy.setY(-5.0f); // sur → azimut=180, marcacion=180
        ctx.tracks.push_back(buoy);

        ctx.ownShip.speedKnots = 9.8747;

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active          = true;
        ctx.canalSession.columnas[0].trackId          = 1;
        ctx.canalSession.columnas[0].hasTriggeredAlarm = true; // simulamos ciclo anterior

        CanalService service(&ctx);
        service.update();

        // ASSERT: la columna se resetea (volvio a sus valores por defecto)
        QVERIFY2(!ctx.canalSession.columnas[0].active, "La columna debe resetearse al superar el traves");
        // ASSERT: la sesion global sigue activa ESTE tick (el flag 'lagea' un ciclo)
        QVERIFY2(ctx.canalSession.active,
                 "NOTA: 'active' queda en true un ciclo mas porque anyColumnActive se marca true "
                 "ANTES de evaluar el reset por traves (a diferencia del reset por track borrado)");
    }

    void test_CANSVC_14_Update_NoReseteaSiSigueEnVentanaAunActivada() {
        // ARRANGE: hasTriggeredAlarm ya en true, pero la boya SIGUE dentro de la ventana (marcacion=90)
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f); own.setCourseDeg(0.0);
        ctx.tracks.push_back(own);
        Track buoy; buoy.setId(1); buoy.setX(5.0f); buoy.setY(0.0f);
        ctx.tracks.push_back(buoy);

        ctx.ownShip.speedKnots = 9.8747;

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active           = true;
        ctx.canalSession.columnas[0].trackId           = 1;
        ctx.canalSession.columnas[0].hasTriggeredAlarm = true;

        CanalService service(&ctx);
        service.update();

        // ASSERT: no debe resetearse mientras siga dentro de la ventana
        QVERIFY2(ctx.canalSession.columnas[0].active, "No debe resetearse mientras la boya siga en la ventana de alarma");
        QVERIFY(ctx.canalSession.columnas[0].isAlarmActive);
        QVERIFY(ctx.canalSession.columnas[0].hasTriggeredAlarm);
    }

    void test_CANSVC_15_Update_MultiplesColumnas_Independientes() {
        // ARRANGE: columna A → boya al norte (fuera de ventana), columna B → boya al este (en ventana)
        CommandContext ctx;
        Track own; own.setId(0); own.setX(0.0f); own.setY(0.0f); own.setCourseDeg(0.0);
        ctx.tracks.push_back(own);
        Track buoyA; buoyA.setId(1); buoyA.setX(0.0f); buoyA.setY(5.0f);
        ctx.tracks.push_back(buoyA);
        Track buoyB; buoyB.setId(2); buoyB.setX(5.0f); buoyB.setY(0.0f);
        ctx.tracks.push_back(buoyB);

        ctx.ownShip.speedKnots = 9.8747;

        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;
        ctx.canalSession.columnas[1].active  = true;
        ctx.canalSession.columnas[1].trackId = 2;

        CanalService service(&ctx);
        service.update();

        // ASSERT: cada columna calcula independientemente su propio estado
        QVERIFY2(!ctx.canalSession.columnas[0].isAlarmActive, "Columna A (norte) no deberia estar en alarma");
        QVERIFY2(ctx.canalSession.columnas[1].isAlarmActive,  "Columna B (este) deberia estar en alarma");
        QVERIFY(ctx.canalSession.active);
    }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para casos de startSession (data-driven)
    // ─────────────────────────────────────────────────────────────
    void runStartSessionTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        CanalService service(&ctx);

        CanalOperationResult res = service.startSession(configFromJson(in));

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'").arg(id, ex["messageContains"].toString(), res.message)));
    }
};

QTEST_APPLESS_MAIN(TestCanalService)
#include "test_canalservice.moc"
