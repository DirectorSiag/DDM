#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "fondeoService.h"
#include "commandContext.h"
#include "model/entities/track.h"

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/fondeoservice_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["fondeoServiceTestCases"].toArray();
}

// ─────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────
static FondeoConfig configFromJson(const QJsonObject& in)
{
    FondeoConfig cfg;
    cfg.useTrack = in["useTrack"].toBool(false);
    cfg.useGms   = in["useGms"].toBool(false);

    cfg.trackId  = in["trackId"].toInt(0);
    cfg.trackAz  = in["trackAz"].toDouble(0.0);
    cfg.trackDt  = in["trackDt"].toDouble(0.0);

    cfg.pfLatDeg = in["pfLatDeg"].toInt(0); cfg.pfLatMin = in["pfLatMin"].toInt(0); cfg.pfLatSec = in["pfLatSec"].toDouble(0.0);
    cfg.pfLonDeg = in["pfLonDeg"].toInt(0); cfg.pfLonMin = in["pfLonMin"].toInt(0); cfg.pfLonSec = in["pfLonSec"].toDouble(0.0);

    cfg.paAz = in["paAz"].toDouble(0.0);
    cfg.paDt = in["paDt"].toDouble(0.0);
    cfg.r1   = in["r1"].toDouble(0.0); cfg.r2 = in["r2"].toDouble(0.0); cfg.r3 = in["r3"].toDouble(0.0);
    cfg.r4   = in["r4"].toDouble(0.0); cfg.r5 = in["r5"].toDouble(0.0);

    return cfg;
}

static void setupContext(CommandContext& ctx, const QJsonObject& in)
{
    if (in.contains("trackId") && in.contains("trackX")) {
        Track t;
        t.setId(in["trackId"].toInt());
        t.setX(static_cast<float>(in["trackX"].toDouble()));
        t.setY(static_cast<float>(in["trackY"].toDouble()));
        ctx.tracks.push_back(t);
    }
    if (in["injectOwnShipGeo"].toBool(false)) {
        ctx.ownShip.valid        = true;
        ctx.ownShip.latitudeDeg  = in["ownShipLat"].toDouble();
        ctx.ownShip.longitudeDeg = in["ownShipLon"].toDouble();
    }
}

class TestFondeoService : public QObject {
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

    // ── startSession ─────────────────────────────────────────────

    void test_SVC_01_StartSession_Exitoso_ModoTrack()          { runStartSessionTest("SVC_01_StartSession_Exitoso_ModoTrack"); }
    void test_SVC_02_StartSession_Exitoso_ModoGMS()            { runStartSessionTest("SVC_02_StartSession_Exitoso_ModoGMS"); }
    void test_SVC_03_StartSession_Error_AmbosModos()           { runStartSessionTest("SVC_03_StartSession_Error_AmbosModos"); }
    void test_SVC_04_StartSession_Error_NingunModo()           { runStartSessionTest("SVC_04_StartSession_Error_NingunModo"); }
    void test_SVC_05_StartSession_Error_TrackInexistente()     { runStartSessionTest("SVC_05_StartSession_Error_TrackInexistente"); }
    void test_SVC_06_StartSession_Error_PaAz_Negativo()        { runStartSessionTest("SVC_06_StartSession_Error_PaAz_Negativo"); }
    void test_SVC_07_StartSession_Error_PaAz_Mayor360()        { runStartSessionTest("SVC_07_StartSession_Error_PaAz_Mayor360"); }
    void test_SVC_08_StartSession_Error_PaDt_Cero()            { runStartSessionTest("SVC_08_StartSession_Error_PaDt_Cero"); }
    void test_SVC_09_StartSession_Error_RadiosNoDecrecientes() { runStartSessionTest("SVC_09_StartSession_Error_RadiosNoDecrecientes"); }
    void test_SVC_10_StartSession_Error_R5_Cero()              { runStartSessionTest("SVC_10_StartSession_Error_R5_Cero"); }
    void test_SVC_11_StartSession_Error_GMS_SinCoordenadasBP() { runStartSessionTest("SVC_11_StartSession_Error_GMS_SinCoordenadasBP"); }

    void test_SVC_12_StartSession_ActivaLaSesion() {
        QJsonObject tc = findCase("SVC_12_StartSession_ActivaLaSesion");
        QJsonObject in = tc["inputs"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        FondeoService service(&ctx);
        service.startSession(configFromJson(in));

        QVERIFY(ctx.fondeoSession.active);
        QVERIFY(!ctx.fondeoSession.paAlcanzado);
    }

    void test_SVC_13_StartSession_PersisteFondeoConfig() {
        QJsonObject tc = findCase("SVC_13_StartSession_PersisteFondeoConfig");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        FondeoService service(&ctx);
        service.startSession(configFromJson(in));

        QCOMPARE(ctx.fondeoSession.config.r1,   ex["r1"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.r2,   ex["r2"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.r3,   ex["r3"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.r4,   ex["r4"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.r5,   ex["r5"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.paAz, ex["paAz"].toDouble());
        QCOMPARE(ctx.fondeoSession.config.paDt, ex["paDt"].toDouble());
    }

    void test_SVC_14_StartSession_ResetSesionPrevia() {
        QJsonObject tc = findCase("SVC_14_StartSession_ResetSesionPrevia");
        QJsonObject in = tc["inputs"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        FondeoService service(&ctx);
        service.startSession(configFromJson(in));
        ctx.fondeoSession.paAlcanzado = true; // simulamos avance en la primera sesión

        service.startSession(configFromJson(in)); // segunda sesión

        QVERIFY(!ctx.fondeoSession.paAlcanzado);
        QVERIFY(ctx.fondeoSession.active);
    }

    // ── stopSession ───────────────────────────────────────────────

    void test_SVC_15_StopSession_ConSesionActiva() {
        QJsonObject tc = findCase("SVC_15_StopSession_ConSesionActiva");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        ctx.fondeoSession.active = true;
        FondeoService service(&ctx);

        FondeoOperationResult res = service.stopSession();

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("Esperado: '%1' | Obtenido: '%2'").arg(ex["messageContains"].toString(), res.message)));
        QVERIFY(!ctx.fondeoSession.active);
    }

    void test_SVC_16_StopSession_SinSesionActiva() {
        QJsonObject tc = findCase("SVC_16_StopSession_SinSesionActiva");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        FondeoService service(&ctx);

        FondeoOperationResult res = service.stopSession();

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("Esperado: '%1' | Obtenido: '%2'").arg(ex["messageContains"].toString(), res.message)));
    }

    void test_SVC_17_StopSession_LimpiaTodoElEstado() {
        QJsonObject tc = findCase("SVC_17_StopSession_LimpiaTodoElEstado");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        ctx.fondeoSession.active      = true;
        ctx.fondeoSession.paAlcanzado = true;
        ctx.fondeoSession.distanciaPF = 500.0;
        ctx.fondeoSession.azimutPF    = 90.0;
        ctx.fondeoSession.puntoFondeo = QPointF(3.0, 4.0);

        FondeoService service(&ctx);
        service.stopSession();

        QCOMPARE(ctx.fondeoSession.active,      ex["active"].toBool());
        QCOMPARE(ctx.fondeoSession.paAlcanzado, ex["paAlcanzado"].toBool());
        QCOMPARE(ctx.fondeoSession.distanciaPF, ex["distanciaPF"].toDouble());
        QCOMPARE(ctx.fondeoSession.azimutPF,    ex["azimutPF"].toDouble());
        QCOMPARE(ctx.fondeoSession.puntoFondeo, QPointF(0.0, 0.0));
    }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para casos de startSession (success + message)
    // ─────────────────────────────────────────────────────────────
    void runStartSessionTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        FondeoService service(&ctx);

        FondeoOperationResult res = service.startSession(configFromJson(in));

        QCOMPARE(res.success, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'").arg(id, ex["messageContains"].toString(), res.message)));
    }
};

QTEST_APPLESS_MAIN(TestFondeoService)
#include "test_fondeoservice.moc"
