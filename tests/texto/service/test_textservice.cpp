#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "textService.h"
#include "commandContext.h"
#include "model/entities/track.h"

static constexpr double kEps = 0.01;

// ─────────────────────────────────────────────────────────────────
// Carga el JSON de casos
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/textService_cases.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll())
        .object()["textServiceTestCases"].toArray();
}

// ─────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────
static TextCreateParams paramsFromJson(const QJsonObject& in)
{
    TextCreateParams params;
    params.texto = in["texto"].toString();

    if (in.contains("tamano")) {
        bool ok = false;
        params.fontSize = TextService::stringToFontSize(in["tamano"].toString(), ok);
    }
    if (in.contains("color")) {
        bool ok = false;
        params.fontColor = TextService::stringToColor(in["color"].toString(), ok);
    }
    if (in.contains("fondo")) {
        bool ok = false;
        params.backgroundColor = TextService::stringToColor(in["fondo"].toString(), ok);
    }
    if (in.contains("borde")) {
        bool ok = false;
        params.borderColor = TextService::stringToColor(in["borde"].toString(), ok);
    }

    const QString method = in["method"].toString();
    if (method == QStringLiteral("manual")) {
        params.positionMethod = TextPositionMethod::Manual;
        params.manXDm = in["manX"].toDouble();
        params.manYDm = in["manY"].toDouble();
    } else if (method == QStringLiteral("azimut")) {
        params.positionMethod = TextPositionMethod::AzimutDist;
        params.azimuthDeg   = in["az"].toDouble();
        params.distanceDm   = in["dt"].toDouble();
        params.useVerdadero = in["verdadero"].toBool(true);
    } else if (method == QStringLiteral("latlon")) {
        params.positionMethod = TextPositionMethod::LatLon;
        params.latDecimal = in["lat"].toDouble();
        params.lonDecimal = in["lon"].toDouble();
    } else if (method == QStringLiteral("deltrack")) {
        params.positionMethod = TextPositionMethod::DelTrack;
        params.refTrackId = in["refTrackId"].toInt();
    }

    if (in["asociarAlCrear"].toBool(false)) {
        params.asociarAlCrear   = true;
        params.trackToAssociate = in["trackToAssociate"].toInt();
    }

    return params;
}

// Inyecta en el contexto los tracks/geo que el caso necesite, según
// las claves presentes en el JSON de inputs/setup.
static void setupContext(CommandContext& ctx, const QJsonObject& in)
{
    if (in.contains("bpX")) {
        Track bp;
        bp.setId(0);
        bp.setX(static_cast<float>(in["bpX"].toDouble()));
        bp.setY(static_cast<float>(in["bpY"].toDouble()));
        if (in.contains("bpCourse")) bp.setCourseDeg(in["bpCourse"].toDouble());
        ctx.tracks.push_back(bp);
    }
    if (in.contains("refTrackId") && in.contains("refTrackX")) {
        Track t;
        t.setId(in["refTrackId"].toInt());
        t.setX(static_cast<float>(in["refTrackX"].toDouble()));
        t.setY(static_cast<float>(in["refTrackY"].toDouble()));
        ctx.tracks.push_back(t);
    }
    if (in.contains("trackToAssociate") && in.contains("assocTrackX")) {
        Track t;
        t.setId(in["trackToAssociate"].toInt());
        t.setX(static_cast<float>(in["assocTrackX"].toDouble()));
        t.setY(static_cast<float>(in["assocTrackY"].toDouble()));
        ctx.tracks.push_back(t);
    }
    if (in["injectOwnShipGeo"].toBool(false)) {
        ctx.ownShip.valid        = true;
        ctx.ownShip.latitudeDeg  = in["ownShipLat"].toDouble();
        ctx.ownShip.longitudeDeg = in["ownShipLon"].toDouble();
    }
}

class TestTextService : public QObject {
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

    // ── createLabel — casos genéricos success/error ────────────────

    void test_SVC_01_CreateLabel_Manual_Exitoso()               { runCreateLabelTest("SVC_01_CreateLabel_Manual_Exitoso"); }
    void test_SVC_02_CreateLabel_TextoVacio_Error()              { runCreateLabelTest("SVC_02_CreateLabel_TextoVacio_Error"); }
    void test_SVC_03_CreateLabel_TextoDemasiadoLargo_Error()     { runCreateLabelTest("SVC_03_CreateLabel_TextoDemasiadoLargo_Error"); }
    void test_SVC_04_CreateLabel_ColorIgualFondo_Error()         { runCreateLabelTest("SVC_04_CreateLabel_ColorIgualFondo_Error"); }
    void test_SVC_05_CreateLabel_AzimutDist_Verdadero_Exitoso()  { runCreateLabelTest("SVC_05_CreateLabel_AzimutDist_Verdadero_Exitoso"); }
    void test_SVC_06_CreateLabel_AzimutDist_SinBP_Error()        { runCreateLabelTest("SVC_06_CreateLabel_AzimutDist_SinBP_Error"); }
    void test_SVC_07_CreateLabel_AzimutDist_Relativo_Exitoso()   { runCreateLabelTest("SVC_07_CreateLabel_AzimutDist_Relativo_Exitoso"); }
    void test_SVC_08_CreateLabel_LatLon_Exitoso()                { runCreateLabelTest("SVC_08_CreateLabel_LatLon_Exitoso"); }
    void test_SVC_09_CreateLabel_LatLon_SinGeoBP_Error()         { runCreateLabelTest("SVC_09_CreateLabel_LatLon_SinGeoBP_Error"); }
    void test_SVC_10_CreateLabel_DelTrack_Exitoso()              { runCreateLabelTest("SVC_10_CreateLabel_DelTrack_Exitoso"); }
    void test_SVC_11_CreateLabel_DelTrack_TrackInexistente_Error(){ runCreateLabelTest("SVC_11_CreateLabel_DelTrack_TrackInexistente_Error"); }
    void test_SVC_12_CreateLabel_AsociarAlCrear_TrackInexistente_Error() { runCreateLabelTest("SVC_12_CreateLabel_AsociarAlCrear_TrackInexistente_Error"); }
    void test_SVC_13_CreateLabel_AsociarAlCrear_Exitoso()        { runCreateLabelTest("SVC_13_CreateLabel_AsociarAlCrear_Exitoso"); }

    void runCreateLabelTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        setupContext(ctx, in);
        TextService service(&ctx);

        TextOperationResult res = service.createLabel(paramsFromJson(in));

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── createLabel — persistencia de posición y TN ────────────────

    void test_SVC_14_CreateLabel_PersistePosicionManual() {
        QJsonObject tc = findCase("SVC_14_CreateLabel_PersistePosicionManual");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);
        service.createLabel(paramsFromJson(in));

        QVERIFY2(!ctx.textSession.labels.isEmpty(), "Debia crearse un label");
        const TextLabel& label = ctx.textSession.labels.first();

        QVERIFY2(qAbs(label.currentXDm - ex["x"].toDouble()) < kEps,
                 qPrintable(QString("currentXDm: esperado %1, obtenido %2").arg(ex["x"].toDouble()).arg(label.currentXDm)));
        QVERIFY2(qAbs(label.currentYDm - ex["y"].toDouble()) < kEps,
                 qPrintable(QString("currentYDm: esperado %1, obtenido %2").arg(ex["y"].toDouble()).arg(label.currentYDm)));
    }

    void test_SVC_15_CreateLabel_AsignaTnIncremental() {
        QJsonObject tc = findCase("SVC_15_CreateLabel_AsignaTnIncremental");
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        TextCreateParams p1;
        p1.texto = QStringLiteral("A");
        p1.fontColor = TextColor::Rojo;
        p1.backgroundColor = TextColor::Blanco;
        p1.positionMethod = TextPositionMethod::Manual;
        service.createLabel(p1);

        TextCreateParams p2 = p1;
        p2.texto = QStringLiteral("B");
        service.createLabel(p2);

        QCOMPARE(ctx.textSession.labels.size(), 2);
        QCOMPARE(ctx.textSession.labels.at(0).tn, ex["firstTn"].toInt());
        QCOMPARE(ctx.textSession.labels.at(1).tn, ex["secondTn"].toInt());
    }

    // ── editLabel ────────────────────────────────────────────────

    void test_SVC_16_EditLabel_CambiaTexto_Exitoso()        { runEditLabelTest("SVC_16_EditLabel_CambiaTexto_Exitoso"); }
    void test_SVC_17_EditLabel_NoExiste_Error()              { runEditLabelTest("SVC_17_EditLabel_NoExiste_Error"); }
    void test_SVC_18_EditLabel_ColorIgualFondoFinal_Error()  { runEditLabelTest("SVC_18_EditLabel_ColorIgualFondoFinal_Error"); }
    void test_SVC_19_EditLabel_TamanoInvalido_Error()        { runEditLabelTest("SVC_19_EditLabel_TamanoInvalido_Error"); }

    void runEditLabelTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        int tn = 99; // TN por defecto que no existe, para los casos "NoExiste"
        if (tc["setup"].isObject()) {
            service.createLabel(paramsFromJson(tc["setup"].toObject()));
            tn = ctx.textSession.labels.first().tn;
        }

        QJsonObject editFields = tc["edit"].toObject();
        QMap<QString, QString> fields;
        for (const QString& key : editFields.keys()) {
            fields.insert(key, editFields[key].toString());
        }

        TextOperationResult res = service.editLabel(tn, fields);

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── deleteLabel ──────────────────────────────────────────────

    void test_SVC_20_DeleteLabel_Exitoso()   { runDeleteLabelTest("SVC_20_DeleteLabel_Exitoso"); }
    void test_SVC_21_DeleteLabel_NoExiste_Error() { runDeleteLabelTest("SVC_21_DeleteLabel_NoExiste_Error"); }

    void runDeleteLabelTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        int tn = 99;
        if (tc["setup"].isObject()) {
            service.createLabel(paramsFromJson(tc["setup"].toObject()));
            tn = ctx.textSession.labels.first().tn;
        }

        TextOperationResult res = service.deleteLabel(tn);

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── associateTrack ───────────────────────────────────────────

    void test_SVC_22_AssociateTrack_Exitoso()            { runAssociateTest("SVC_22_AssociateTrack_Exitoso"); }
    void test_SVC_23_AssociateTrack_LabelNoExiste_Error() { runAssociateTest("SVC_23_AssociateTrack_LabelNoExiste_Error"); }
    void test_SVC_24_AssociateTrack_TrackNoExiste_Error() { runAssociateTest("SVC_24_AssociateTrack_TrackNoExiste_Error"); }

    void runAssociateTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject assoc = tc["associate"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        int tn = 99;
        if (tc["setup"].isObject()) {
            service.createLabel(paramsFromJson(tc["setup"].toObject()));
            tn = ctx.textSession.labels.first().tn;
        }

        if (assoc.contains("trackX")) {
            Track t;
            t.setId(assoc["trackId"].toInt());
            t.setX(static_cast<float>(assoc["trackX"].toDouble()));
            t.setY(static_cast<float>(assoc["trackY"].toDouble()));
            ctx.tracks.push_back(t);
        }

        TextOperationResult res = service.associateTrack(tn, assoc["trackId"].toInt());

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── dissociateTrack ──────────────────────────────────────────

    void test_SVC_25_DissociateTrack_Exitoso()        { runDissociateTest("SVC_25_DissociateTrack_Exitoso"); }
    void test_SVC_26_DissociateTrack_NoAsociado_Error(){ runDissociateTest("SVC_26_DissociateTrack_NoAsociado_Error"); }

    void runDissociateTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        service.createLabel(paramsFromJson(tc["setup"].toObject()));
        int tn = ctx.textSession.labels.first().tn;

        if (tc.contains("associate")) {
            QJsonObject assoc = tc["associate"].toObject();
            Track t;
            t.setId(assoc["trackId"].toInt());
            t.setX(static_cast<float>(assoc["trackX"].toDouble()));
            t.setY(static_cast<float>(assoc["trackY"].toDouble()));
            ctx.tracks.push_back(t);
            service.associateTrack(tn, assoc["trackId"].toInt());
        }

        TextOperationResult res = service.dissociateTrack(tn);

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── listLabels / infoLabel ───────────────────────────────────

    void test_SVC_27_ListLabels_Vacio()      { runListLabelsTest("SVC_27_ListLabels_Vacio"); }
    void test_SVC_28_ListLabels_ConTextos()  { runListLabelsTest("SVC_28_ListLabels_ConTextos"); }

    void runListLabelsTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);
        if (tc["setup"].isObject()) {
            service.createLabel(paramsFromJson(tc["setup"].toObject()));
        }

        TextOperationResult res = service.listLabels();

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    void test_SVC_29_InfoLabel_NoExiste_Error() { runInfoLabelTest("SVC_29_InfoLabel_NoExiste_Error"); }
    void test_SVC_30_InfoLabel_Exitoso()        { runInfoLabelTest("SVC_30_InfoLabel_Exitoso"); }

    void runInfoLabelTest(const QString& id) {
        QJsonObject tc = findCase(id);
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextService service(&ctx);

        int tn = 99;
        if (tc["setup"].isObject()) {
            service.createLabel(paramsFromJson(tc["setup"].toObject()));
            tn = ctx.textSession.labels.first().tn;
        }

        TextOperationResult res = service.infoLabel(tn);

        QCOMPARE(res.ok, ex["success"].toBool());
        QVERIFY2(res.message.contains(ex["messageContains"].toString(), Qt::CaseInsensitive),
                 qPrintable(QString("[%1] Esperado: '%2' | Obtenido: '%3'")
                                .arg(id, ex["messageContains"].toString(), res.message)));
    }

    // ── update ───────────────────────────────────────────────────

    void test_SVC_31_Update_RecalculaPosicionAsociada() {
        QJsonObject tc = findCase("SVC_31_Update_RecalculaPosicionAsociada");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        Track t;
        t.setId(in["trackId"].toInt());
        t.setX(static_cast<float>(in["initialTrackX"].toDouble()));
        t.setY(static_cast<float>(in["initialTrackY"].toDouble()));
        ctx.tracks.push_back(t);

        TextLabel label;
        label.tn = 1;
        label.associated        = true;
        label.associatedTrackId = in["trackId"].toInt();
        label.offsetXDm = in["offsetX"].toDouble();
        label.offsetYDm = in["offsetY"].toDouble();
        ctx.textSession.labels.append(label);

        // El track se mueve antes de llamar a update()
        ctx.tracks[0].setX(static_cast<float>(in["movedTrackX"].toDouble()));
        ctx.tracks[0].setY(static_cast<float>(in["movedTrackY"].toDouble()));

        TextService service(&ctx);
        service.update();

        const TextLabel& updated = ctx.textSession.labels.first();
        QVERIFY2(qAbs(updated.currentXDm - ex["x"].toDouble()) < kEps,
                 qPrintable(QString("currentXDm: esperado %1, obtenido %2").arg(ex["x"].toDouble()).arg(updated.currentXDm)));
        QVERIFY2(qAbs(updated.currentYDm - ex["y"].toDouble()) < kEps,
                 qPrintable(QString("currentYDm: esperado %1, obtenido %2").arg(ex["y"].toDouble()).arg(updated.currentYDm)));
    }

    void test_SVC_32_Update_NoAsociado_NoCambia() {
        QJsonObject tc = findCase("SVC_32_Update_NoAsociado_NoCambia");
        QJsonObject in = tc["inputs"].toObject();
        QJsonObject ex = tc["expected"].toObject();

        CommandContext ctx;
        TextLabel label;
        label.tn = 1;
        label.associated = false;
        label.currentXDm = in["x"].toDouble();
        label.currentYDm = in["y"].toDouble();
        ctx.textSession.labels.append(label);

        TextService service(&ctx);
        service.update();

        const TextLabel& unchanged = ctx.textSession.labels.first();
        QCOMPARE(unchanged.currentXDm, ex["x"].toDouble());
        QCOMPARE(unchanged.currentYDm, ex["y"].toDouble());
    }
};

QTEST_APPLESS_MAIN(TestTextService)
#include "test_textservice.moc"