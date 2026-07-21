#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "canalCommand.h"
#include "commandContext.h"
#include "model/entities/track.h"

// Campos confirmados contra iCommand.h y commandContext.h:
// - CommandInvocation.args es QStringList (igual que en FondeoCommand).
// - CommandResult usa .ok (bool) y .message (QString) -- OJO: no es ".success"
//   como en CanalOperationResult/FondeoOperationResult, es un nombre distinto
//   a nivel de la capa Command vs. la capa Service.
// - CommandContext.canalSession y CommandContext.ownShip.speedKnots confirmados.

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/canalcommand_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["canalCommandTestCases"].toArray();
}

// ─────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────
static CommandInvocation invocationFromJson(const QJsonObject& in)
{
    CommandInvocation inv;
    for (const QJsonValue& v : in["args"].toArray())
        inv.args << v.toString();
    return inv;
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

class TestCanalCommand : public QObject {
    Q_OBJECT
    QJsonArray m_cases;
    CanalCommand m_command;

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

    // ── Casos data-driven ────────────────────────────────────────

    void test_CANCMD_01_Info_SinSesionActiva()              { runCommandTest("CANCMD_01_Info_SinSesionActiva"); }
    void test_CANCMD_04_Borrar_SinSesionActiva()             { runCommandTest("CANCMD_04_Borrar_SinSesionActiva"); }
    void test_CANCMD_06_Start_ColumnaA_Exitosa()             { runCommandTest("CANCMD_06_Start_ColumnaA_Exitosa"); }
    void test_CANCMD_07_Start_MultiplesColumnas()            { runCommandTest("CANCMD_07_Start_MultiplesColumnas"); }
    void test_CANCMD_08_Error_SinArgumentos()                { runCommandTest("CANCMD_08_Error_SinArgumentos"); }
    void test_CANCMD_09_Error_ComandoNoReconocido()          { runCommandTest("CANCMD_09_Error_ComandoNoReconocido"); }
    void test_CANCMD_10_Error_Start_SinColumnas()            { runCommandTest("CANCMD_10_Error_Start_SinColumnas"); }
    void test_CANCMD_11_Error_Start_ValorNoEntero_ColumnaA() { runCommandTest("CANCMD_11_Error_Start_ValorNoEntero_ColumnaA"); }
    void test_CANCMD_12_Error_Start_TrackInexistente()       { runCommandTest("CANCMD_12_Error_Start_TrackInexistente"); }

    // ── Casos que requieren estado previo en el contexto ─────────

    void test_CANCMD_02_Info_ConColumnaActiva() {
        // ARRANGE: columna A activa con datos cargados
        CommandContext ctx;
        ctx.canalSession.columnas[0].active          = true;
        ctx.canalSession.columnas[0].trackId          = 7;
        ctx.canalSession.columnas[0].azimutVerdadero  = 45.0;
        ctx.canalSession.columnas[0].distanciaYardas  = 1234.5;
        ctx.canalSession.columnas[0].rumboVerdadero   = 10.0;
        ctx.canalSession.columnas[0].timeToArrivalMin = 5.5;
        ctx.canalSession.columnas[0].etaValid          = true;
        ctx.canalSession.columnas[0].isAlarmActive     = true;

        CommandInvocation inv;
        inv.args << "--info";

        // ACT
        CommandResult res = m_command.execute(inv, ctx);

        // ASSERT
        QVERIFY(res.ok);
        QVERIFY2(res.message.contains("Columna A"), "El mensaje debe mostrar la columna A");
        QVERIFY2(res.message.contains("7"), "El mensaje debe mostrar el track id 7");
        QVERIFY2(res.message.contains("ON"), "El mensaje debe mostrar la alarma como ON");
    }

    void test_CANCMD_03_Borrar_ConSesionActiva() {
        // ARRANGE
        CommandContext ctx;
        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CommandInvocation inv;
        inv.args << "--borrar";

        // ACT
        CommandResult res = m_command.execute(inv, ctx);

        // ASSERT
        QVERIFY(res.ok);
        QVERIFY2(res.message.contains("borradas", Qt::CaseInsensitive),
                 qPrintable(QString("Mensaje obtenido: %1").arg(res.message)));
        QVERIFY(!ctx.canalSession.active);
    }

    void test_CANCMD_05_Stop_AliasDeBorrar() {
        // ARRANGE: --stop debe comportarse igual que --borrar
        CommandContext ctx;
        ctx.canalSession.active = true;
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CommandInvocation inv;
        inv.args << "--stop";

        // ACT
        CommandResult res = m_command.execute(inv, ctx);

        // ASSERT
        QVERIFY(res.ok);
        QVERIFY2(res.message.contains("borradas", Qt::CaseInsensitive),
                 "El alias --stop debe producir el mismo resultado que --borrar");
        QVERIFY(!ctx.canalSession.active);
    }

    void test_CANCMD_13_Error_Start_ColumnaEnUso() {
        // ARRANGE: columna A ya activa, se intenta re-setearla
        CommandContext ctx;
        Track t; t.setId(5); t.setX(0.0f); t.setY(5.0f);
        ctx.tracks.push_back(t);
        ctx.canalSession.columnas[0].active  = true;
        ctx.canalSession.columnas[0].trackId = 1;

        CommandInvocation inv;
        inv.args << "--start" << "--a=5";

        // ACT
        CommandResult res = m_command.execute(inv, ctx);

        // ASSERT
        QVERIFY(!res.ok);
        QVERIFY2(res.message.contains("en uso", Qt::CaseInsensitive),
                 qPrintable(QString("Mensaje obtenido: %1").arg(res.message)));
    }

    // ─────────────────────────────────────────────────────────────
    // Helper compartido para casos data-driven
    // ─────────────────────────────────────────────────────────────
    void runCommandTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        CommandInvocation inv = invocationFromJson(in);

        CommandResult res = m_command.execute(inv, ctx);

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'").arg(id, ex["messageContains"].toString(), res.message)));
    }
};

QTEST_APPLESS_MAIN(TestCanalCommand)
#include "test_canalcommand.moc"
