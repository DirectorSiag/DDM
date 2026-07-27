#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "borneoService.h"
#include "borneoSessionState.h"
#include "commandContext.h"

static constexpr double kEps = 0.001;

// ─────────────────────────────────────────────────────────────────
// Carga del JSON
// ─────────────────────────────────────────────────────────────────
static QJsonArray loadCases()
{
    QFile file(":/json/json/borneoservice_cases.json");

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    return QJsonDocument::fromJson(file.readAll())
        .object()["borneoServiceTestCases"]
        .toArray();
}

static BorneoConfig configFromJson(const QJsonObject& in)
{
    BorneoConfig config;

    config.eslora =
        in["eslora"].toDouble(0.0);

    config.grilletes =
        in["grilletes"].toInt(0);

    config.profundidad =
        in["profundidad"].toDouble(0.0);

    return config;
}

class TestBorneoService : public QObject
{
    Q_OBJECT

    QJsonArray m_cases;

    QJsonObject findCase(const QString& id) const
    {
        for (const QJsonValue& value : m_cases) {
            const QJsonObject object = value.toObject();

            if (object["id"].toString() == id) {
                return object;
            }
        }

        return {};
    }

private slots:

    void initTestCase()
    {
        m_cases = loadCases();

        QVERIFY2(
            !m_cases.isEmpty(),
            "JSON de casos no cargado. Revisa el resources_test.qrc"
        );
    }

    // ── startSession manual ───────────────────────────────────────

    void test_SVC_01_StartSession_Exitoso_Manual()
    {
        runStartSessionTest(
            "SVC_01_StartSession_Exitoso_Manual"
        );
    }

    void test_SVC_03_StartSession_Error_EsloraCero()
    {
        runStartSessionTest(
            "SVC_03_StartSession_Error_EsloraCero"
        );
    }

    void test_SVC_04_StartSession_Error_EsloraNegativa()
    {
        runStartSessionTest(
            "SVC_04_StartSession_Error_EsloraNegativa"
        );
    }

    void test_SVC_05_StartSession_Error_GrilletesNegativos()
    {
        runStartSessionTest(
            "SVC_05_StartSession_Error_GrilletesNegativos"
        );
    }

    void test_SVC_06_StartSession_Error_ProfundidadNegativa()
    {
        runStartSessionTest(
            "SVC_06_StartSession_Error_ProfundidadNegativa"
        );
    }

    void test_SVC_07_StartSession_RadioExactamenteCero()
    {
        runStartSessionTest(
            "SVC_07_StartSession_RadioExactamenteCero"
        );
    }

    void test_SVC_08_StartSession_RadioNegativo()
    {
        runStartSessionTest(
            "SVC_08_StartSession_RadioNegativo"
        );
    }

    // ── startSessionByClase ───────────────────────────────────────

    void test_SVC_02_StartSessionByClase_Exitoso()
    {
        runStartByClaseTest(
            "SVC_02_StartSessionByClase_Exitoso"
        );
    }

    void test_SVC_09_StartSessionByClase_ClaseInexistente()
    {
        runStartByClaseTest(
            "SVC_09_StartSessionByClase_ClaseInexistente"
        );
    }

    // ── Persistencia del estado ───────────────────────────────────

    void test_SVC_10_StartSession_ActivaSesion()
    {
        const QJsonObject tc =
            findCase("SVC_10_StartSession_ActivaSesion");

        const QJsonObject in =
            tc["inputs"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSession(configFromJson(in));

        QVERIFY(result.success);
        QVERIFY(ctx.borneoSession.active);
    }

    void test_SVC_11_StartSession_PersisteConfig()
    {
        const QJsonObject tc =
            findCase("SVC_11_StartSession_PersisteConfig");

        const QJsonObject in =
            tc["inputs"].toObject();

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSession(configFromJson(in));

        QVERIFY(result.success);

        QCOMPARE(
            ctx.borneoSession.config.eslora,
            ex["eslora"].toDouble()
        );

        QCOMPARE(
            ctx.borneoSession.config.grilletes,
            ex["grilletes"].toInt()
        );

        QCOMPARE(
            ctx.borneoSession.config.profundidad,
            ex["profundidad"].toDouble()
        );
    }

    void test_SVC_12_StartSession_PersisteRadio()
    {
        const QJsonObject tc =
            findCase("SVC_12_StartSession_PersisteRadio");

        const QJsonObject in =
            tc["inputs"].toObject();

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSession(configFromJson(in));

        QVERIFY(result.success);

        QVERIFY2(
            qAbs(
                ctx.borneoSession.radioCalculado -
                ex["radio"].toDouble()
            ) < kEps,
            qPrintable(
                QString("Radio esperado: %1 | obtenido: %2")
                    .arg(ex["radio"].toDouble())
                    .arg(ctx.borneoSession.radioCalculado)
            )
        );
    }

    void test_SVC_13_StartSessionByClase_PersisteEsloraResuelta()
    {
        const QJsonObject tc =
            findCase(
                "SVC_13_StartSessionByClase_PersisteEsloraResuelta"
            );

        const QJsonObject in =
            tc["inputs"].toObject();

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSessionByClase(
                in["clase"].toString(),
                in["grilletes"].toInt(),
                in["profundidad"].toDouble()
            );

        QVERIFY(result.success);

        QVERIFY2(
            qAbs(
                ctx.borneoSession.config.eslora -
                ex["eslora"].toDouble()
            ) < kEps,
            "La eslora resuelta por catalogo no coincide"
        );

        QVERIFY2(
            qAbs(
                ctx.borneoSession.radioCalculado -
                ex["radio"].toDouble()
            ) < kEps,
            "El radio calculado no coincide"
        );
    }

    // ── Reinvocación de sesión ────────────────────────────────────

    void test_SVC_14_StartSession_SobrescribeSesionPrevia()
    {
        const QJsonObject tc =
            findCase(
                "SVC_14_StartSession_SobrescribeSesionPrevia"
            );

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        BorneoConfig first;
        first.eslora = 90.0;
        first.grilletes = 2;
        first.profundidad = 25.0;

        QVERIFY(service.startSession(first).success);

        BorneoConfig second;
        second.eslora = 100.0;
        second.grilletes = 1;
        second.profundidad = 20.0;

        QVERIFY(service.startSession(second).success);

        QCOMPARE(
            ctx.borneoSession.active,
            ex["active"].toBool()
        );

        QCOMPARE(
            ctx.borneoSession.config.eslora,
            ex["eslora"].toDouble()
        );

        QCOMPARE(
            ctx.borneoSession.config.grilletes,
            ex["grilletes"].toInt()
        );

        QCOMPARE(
            ctx.borneoSession.config.profundidad,
            ex["profundidad"].toDouble()
        );

        QVERIFY2(
            qAbs(
                ctx.borneoSession.radioCalculado -
                ex["radio"].toDouble()
            ) < kEps,
            "La segunda sesion no reemplazo correctamente el radio"
        );
    }

    // ── Fallos y persistencia ─────────────────────────────────────

    void test_SVC_15_StartFallido_NoActivaSesion()
    {
        const QJsonObject tc =
            findCase(
                "SVC_15_StartFallido_NoActivaSesion"
            );

        const QJsonObject in =
            tc["inputs"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSession(configFromJson(in));

        QVERIFY(!result.success);
        QVERIFY(!ctx.borneoSession.active);
    }

    void test_SVC_16_StartFallido_NoModificaSesionPrevia()
    {
        const QJsonObject tc =
            findCase(
                "SVC_16_StartFallido_NoModificaSesionPrevia"
            );

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        BorneoConfig valid;
        valid.eslora = 90.0;
        valid.grilletes = 2;
        valid.profundidad = 25.0;

        QVERIFY(service.startSession(valid).success);

        BorneoConfig invalid;
        invalid.eslora = 20.0;
        invalid.grilletes = 0;
        invalid.profundidad = 100.0;

        const BorneoOperationResult result =
            service.startSession(invalid);

        QVERIFY(!result.success);

        QCOMPARE(
            ctx.borneoSession.active,
            ex["active"].toBool()
        );

        QCOMPARE(
            ctx.borneoSession.config.eslora,
            ex["eslora"].toDouble()
        );

        QVERIFY2(
            qAbs(
                ctx.borneoSession.radioCalculado -
                ex["radio"].toDouble()
            ) < kEps,
            "Una operacion fallida modifico el radio anterior"
        );
    }

    // ── stopSession ───────────────────────────────────────────────

    void test_SVC_17_StopSession_ConSesionActiva()
    {
        const QJsonObject tc =
            findCase(
                "SVC_17_StopSession_ConSesionActiva"
            );

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        ctx.borneoSession.active = true;

        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.stopSession();

        QCOMPARE(
            result.success,
            ex["success"].toBool()
        );

        QVERIFY2(
            result.message.contains(
                ex["messageContains"].toString(),
                Qt::CaseInsensitive
            ),
            qPrintable(result.message)
        );

        QCOMPARE(
            ctx.borneoSession.active,
            ex["active"].toBool()
        );
    }

    void test_SVC_18_StopSession_SinSesionActiva()
    {
        const QJsonObject tc =
            findCase(
                "SVC_18_StopSession_SinSesionActiva"
            );

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.stopSession();

        QCOMPARE(
            result.success,
            ex["success"].toBool()
        );

        QVERIFY2(
            result.message.contains(
                ex["messageContains"].toString(),
                Qt::CaseInsensitive
            ),
            qPrintable(result.message)
        );
    }

    void test_SVC_19_StopSession_ConservaDatosActuales()
    {
        const QJsonObject tc =
            findCase(
                "SVC_19_StopSession_ConservaDatosActuales"
            );

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        BorneoConfig config;
        config.eslora = 90.0;
        config.grilletes = 2;
        config.profundidad = 25.0;

        QVERIFY(service.startSession(config).success);
        QVERIFY(service.stopSession().success);

        QCOMPARE(
            ctx.borneoSession.active,
            ex["active"].toBool()
        );

        QCOMPARE(
            ctx.borneoSession.config.eslora,
            ex["eslora"].toDouble()
        );

        QCOMPARE(
            ctx.borneoSession.config.grilletes,
            ex["grilletes"].toInt()
        );

        QCOMPARE(
            ctx.borneoSession.config.profundidad,
            ex["profundidad"].toDouble()
        );

        QVERIFY2(
            qAbs(
                ctx.borneoSession.radioCalculado -
                ex["radio"].toDouble()
            ) < kEps,
            "stopSession modifico radioCalculado"
        );
    }

    // ─────────────────────────────────────────────────────────────
    // Helpers compartidos
    // ─────────────────────────────────────────────────────────────

    void runStartSessionTest(const QString& id)
    {
        const QJsonObject tc = findCase(id);

        QVERIFY2(
            !tc.isEmpty(),
            qPrintable(
                QString("No se encontro el caso JSON: %1").arg(id)
            )
        );

        const QJsonObject in =
            tc["inputs"].toObject();

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSession(configFromJson(in));

        QCOMPARE(
            result.success,
            ex["success"].toBool()
        );

        QVERIFY2(
            result.message.contains(
                ex["messageContains"].toString(),
                Qt::CaseInsensitive
            ),
            qPrintable(
                QString(
                    "[%1] Esperado: '%2' | Obtenido: '%3'"
                )
                    .arg(
                        id,
                        ex["messageContains"].toString(),
                        result.message
                    )
            )
        );
    }

    void runStartByClaseTest(const QString& id)
    {
        const QJsonObject tc = findCase(id);

        QVERIFY2(
            !tc.isEmpty(),
            qPrintable(
                QString("No se encontro el caso JSON: %1").arg(id)
            )
        );

        const QJsonObject in =
            tc["inputs"].toObject();

        const QJsonObject ex =
            tc["expected"].toObject();

        CommandContext ctx;
        BorneoService service(&ctx);

        const BorneoOperationResult result =
            service.startSessionByClase(
                in["clase"].toString(),
                in["grilletes"].toInt(),
                in["profundidad"].toDouble()
            );

        QCOMPARE(
            result.success,
            ex["success"].toBool()
        );

        QVERIFY2(
            result.message.contains(
                ex["messageContains"].toString(),
                Qt::CaseInsensitive
            ),
            qPrintable(
                QString(
                    "[%1] Esperado: '%2' | Obtenido: '%3'"
                )
                    .arg(
                        id,
                        ex["messageContains"].toString(),
                        result.message
                    )
            )
        );
    }
};

QTEST_APPLESS_MAIN(TestBorneoService)

#include "test_borneoservice.moc"