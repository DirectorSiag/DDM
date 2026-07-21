#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "textCommand.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"
#include "model/entities/track.h"
#include "model/texto/textLabel.h"

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/textCommand_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["textCommandTestCases"].toArray();
}

class TestTextCommand : public QObject {
    Q_OBJECT
    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const {
        for (const QJsonValue& v : m_cases)
            if (v.toObject()["id"].toString() == id) return v.toObject();
        return {};
    }

    // Ejecuta el comando de un caso contra un CommandContext ya preparado.
    void runCase(const QString& id, CommandContext& ctx) {
        QJsonObject tc = findCase(id);
        QVERIFY2(!tc.isEmpty(), qPrintable("Caso no encontrado en JSON: " + id));

        const QString commandLine = tc["commandLine"].toString();
        const bool expectedSuccess = tc["expectedSuccess"].toBool();
        const QString expectedMsg = tc["expectedMessageContains"].toString();

        CommandParser parser;
        CommandInvocation inv;
        QString parseError;
        bool parseOk = parser.parse(commandLine, inv, parseError);
        QVERIFY2(parseOk, qPrintable("El CommandParser fallo: " + parseError));

        TextCommand cmd;
        CommandResult res = cmd.execute(inv, ctx);

        QCOMPARE(res.ok, expectedSuccess);
        bool containsMsg = res.message.contains(expectedMsg, Qt::CaseInsensitive);
        if (!containsMsg) {
            qDebug() << "\n[" << id << "] MENSAJE ESPERADO :" << expectedMsg;
            qDebug() << "[" << id << "] MENSAJE OBTENIDO :" << res.message << "\n";
        }
        QVERIFY2(containsMsg, "El mensaje devuelto por el comando no contiene el texto esperado.");
    }

    // Inyecta un label manual con TN=1, listo para editar/borrar/info/altrack/deltrack.
    void seedLabel(CommandContext& ctx) {
        TextLabel label;
        label.tn = 1;
        label.texto = QStringLiteral("SEED");
        label.fontColor = TextColor::Rojo;
        label.backgroundColor = TextColor::Blanco;
        label.positionMethod = TextPositionMethod::Manual;
        label.baseXDm = label.currentXDm = 0.0;
        label.baseYDm = label.currentYDm = 0.0;
        ctx.textSession.labels.append(label);
        ctx.textSession.nextTn = 2;
    }

    void seedTrack(CommandContext& ctx, int id, float x, float y) {
        Track t;
        t.setId(id);
        t.setX(x);
        t.setY(y);
        ctx.tracks.push_back(t);
    }

private slots:
    void initTestCase() {
        m_cases = loadCases();
        QVERIFY2(!m_cases.isEmpty(), "JSON de casos no cargado. Revisa el .qrc");
    }

    // ── --nuevo: casos sin dependencias externas ────────────────────

    void test_CMD_01_Nuevo_Manual_Exitoso()      { CommandContext ctx; runCase("CMD_01_Nuevo_Manual_Exitoso", ctx); }
    void test_CMD_02_Error_SinArgumentos()       { CommandContext ctx; runCase("CMD_02_Error_SinArgumentos", ctx); }
    void test_CMD_03_Error_Nuevo_Sin_Texto()     { CommandContext ctx; runCase("CMD_03_Error_Nuevo_Sin_Texto", ctx); }
    void test_CMD_04_Error_Nuevo_Sin_Metodo()    { CommandContext ctx; runCase("CMD_04_Error_Nuevo_Sin_Metodo", ctx); }
    void test_CMD_05_Error_Nuevo_DosMetodos()    { CommandContext ctx; runCase("CMD_05_Error_Nuevo_DosMetodos", ctx); }
    void test_CMD_06_Error_Man_FormatoInvalido() { CommandContext ctx; runCase("CMD_06_Error_Man_FormatoInvalido", ctx); }
    void test_CMD_07_Error_Man_NoNumerico()      { CommandContext ctx; runCase("CMD_07_Error_Man_NoNumerico", ctx); }
    void test_CMD_08_Error_Az_SinDt()            { CommandContext ctx; runCase("CMD_08_Error_Az_SinDt", ctx); }
    void test_CMD_09_Error_Az_SinVR()            { CommandContext ctx; runCase("CMD_09_Error_Az_SinVR", ctx); }
    void test_CMD_10_Error_Az_VyR_Simultaneos()  { CommandContext ctx; runCase("CMD_10_Error_Az_VyR_Simultaneos", ctx); }
    void test_CMD_13_Error_Tamano_Invalido()     { CommandContext ctx; runCase("CMD_13_Error_Tamano_Invalido", ctx); }
    void test_CMD_14_Error_Color_Invalido()      { CommandContext ctx; runCase("CMD_14_Error_Color_Invalido", ctx); }
    void test_CMD_15_Error_Borde_Invalido()      { CommandContext ctx; runCase("CMD_15_Error_Borde_Invalido", ctx); }
    void test_CMD_16_Error_Fondo_Invalido()      { CommandContext ctx; runCase("CMD_16_Error_Fondo_Invalido", ctx); }
    void test_CMD_17_Error_Lat_SinLon()          { CommandContext ctx; runCase("CMD_17_Error_Lat_SinLon", ctx); }
    void test_CMD_18_Error_Lat_FormatoInvalido() { CommandContext ctx; runCase("CMD_18_Error_Lat_FormatoInvalido", ctx); }
    void test_CMD_19_Error_DelTrack_NoNumerico() { CommandContext ctx; runCase("CMD_19_Error_DelTrack_NoNumerico", ctx); }
    void test_CMD_21_Error_AlTrack_NoNumerico()  { CommandContext ctx; runCase("CMD_21_Error_AlTrack_NoNumerico", ctx); }
    void test_CMD_23_Error_Editar_TnNoNumerico() { CommandContext ctx; runCase("CMD_23_Error_Editar_TnNoNumerico", ctx); }
    void test_CMD_25_Error_Borrar_TnNoNumerico() { CommandContext ctx; runCase("CMD_25_Error_Borrar_TnNoNumerico", ctx); }
    void test_CMD_29_Error_Info_TnNoNumerico()   { CommandContext ctx; runCase("CMD_29_Error_Info_TnNoNumerico", ctx); }
    void test_CMD_33_Error_Flag_No_Reconocido()  { CommandContext ctx; runCase("CMD_33_Error_Flag_No_Reconocido", ctx); }

    // ── --nuevo --az: requiere el BP (track id=0) en contexto ──────

    void test_CMD_11_Camino_Exitoso_Az_Verdadero() {
        CommandContext ctx;
        seedTrack(ctx, 0, 0.0f, 0.0f);
        runCase("CMD_11_Camino_Exitoso_Az_Verdadero", ctx);
    }

    void test_CMD_12_Camino_Exitoso_Az_Relativo() {
        CommandContext ctx;
        seedTrack(ctx, 0, 0.0f, 0.0f);
        runCase("CMD_12_Camino_Exitoso_Az_Relativo", ctx);
    }

    // ── --deltrack como método de posición: requiere track de referencia ──

    void test_CMD_20_Camino_Exitoso_DelTrack() {
        CommandContext ctx;
        seedTrack(ctx, 5, 2.0f, 2.0f);
        runCase("CMD_20_Camino_Exitoso_DelTrack", ctx);
    }

    // ── --nuevo con --altrack: requiere el track a asociar ──────────

    void test_CMD_22_Camino_Exitoso_Nuevo_Con_AlTrack() {
        CommandContext ctx;
        seedTrack(ctx, 9, 1.0f, 1.0f);
        runCase("CMD_22_Camino_Exitoso_Nuevo_Con_AlTrack", ctx);
    }

    // ── --editar / --borrar / --info: requieren un label preexistente ──

    void test_CMD_24_Camino_Exitoso_Editar() {
        CommandContext ctx;
        seedLabel(ctx);
        runCase("CMD_24_Camino_Exitoso_Editar", ctx);
    }

    void test_CMD_26_Camino_Exitoso_Borrar() {
        CommandContext ctx;
        seedLabel(ctx);
        runCase("CMD_26_Camino_Exitoso_Borrar", ctx);
    }

    void test_CMD_30_Camino_Exitoso_Info() {
        CommandContext ctx;
        seedLabel(ctx);
        runCase("CMD_30_Camino_Exitoso_Info", ctx);
    }

    // ── --altrack/--tn y --deltrack/--tn standalone ──────────────────

    void test_CMD_27_Camino_Exitoso_AlTrack_Standalone() {
        CommandContext ctx;
        seedLabel(ctx);
        seedTrack(ctx, 7, 3.0f, 3.0f);
        runCase("CMD_27_Camino_Exitoso_AlTrack_Standalone", ctx);
    }

    void test_CMD_28_Camino_Exitoso_DelTrack_Standalone() {
        CommandContext ctx;
        seedLabel(ctx);
        seedTrack(ctx, 7, 3.0f, 3.0f);
        // Asociamos primero para poder desasociar (ejecuta el comando de asociación real)
        CommandParser parser;
        CommandInvocation inv;
        QString err;
        parser.parse(QStringLiteral("texto --altrack=7 --tn=1"), inv, err);
        TextCommand cmd;
        cmd.execute(inv, ctx);

        runCase("CMD_28_Camino_Exitoso_DelTrack_Standalone", ctx);
    }

    // ── --list ───────────────────────────────────────────────────────

    void test_CMD_31_Camino_Exitoso_List_Vacio() {
        CommandContext ctx;
        runCase("CMD_31_Camino_Exitoso_List_Vacio", ctx);
    }

    void test_CMD_32_Camino_Exitoso_List_ConDatos() {
        CommandContext ctx;
        seedLabel(ctx);
        runCase("CMD_32_Camino_Exitoso_List_ConDatos", ctx);
    }
};

QTEST_APPLESS_MAIN(TestTextCommand)
#include "test_textcommand.moc"