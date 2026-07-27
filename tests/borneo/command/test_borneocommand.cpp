#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "borneoCommand.h"
#include "borneoService.h"
#include "borneoSessionState.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"

class TestBorneoCommand : public QObject
{
    Q_OBJECT

private slots:

    void testComandosJson_data()
    {
        QTest::addColumn<QString>("commandLine");
        QTest::addColumn<bool>("setupActiveSession");
        QTest::addColumn<bool>("expectedSuccess");
        QTest::addColumn<QString>("expectedMessageContains");

        QFile file(":/jsons/json/borneocommand_cases.json");

        if (!file.open(QIODevice::ReadOnly)) {
            QFAIL(
                "No se pudo abrir el archivo JSON de pruebas. "
                "Revisa el resources_test.qrc"
            );
        }

        const QByteArray data = file.readAll();
        const QJsonDocument doc =
            QJsonDocument::fromJson(data);

        const QJsonArray cases =
            doc.object()["borneoCommandTestCases"].toArray();

        QVERIFY2(
            !cases.isEmpty(),
            "El JSON no contiene casos de prueba"
        );

        for (const QJsonValue& value : cases) {
            const QJsonObject obj = value.toObject();

            const QString id =
                obj["id"].toString();

            const QString commandLine =
                obj["commandLine"].toString();

            const bool setupActiveSession =
                obj["setupActiveSession"].toBool(false);

            const bool expectedSuccess =
                obj["expectedSuccess"].toBool();

            const QString expectedMessageContains =
                obj["expectedMessageContains"].toString();

            QTest::newRow(id.toStdString().c_str())
                << commandLine
                << setupActiveSession
                << expectedSuccess
                << expectedMessageContains;
        }
    }

    void testComandosJson()
    {
        QFETCH(QString, commandLine);
        QFETCH(bool, setupActiveSession);
        QFETCH(bool, expectedSuccess);
        QFETCH(QString, expectedMessageContains);

        // ─────────────────────────────────────────────────────────
        // ARRANGE
        // ─────────────────────────────────────────────────────────

        CommandParser parser;
        CommandInvocation invocation;
        QString parseError;

        const bool parseOk =
            parser.parse(
                commandLine,
                invocation,
                parseError
            );

        QVERIFY2(
            parseOk,
            qPrintable(
                QString(
                    "CommandParser fallo leyendo '%1': %2"
                )
                    .arg(commandLine, parseError)
            )
        );

        CommandContext ctx;

        // Algunos casos necesitan partir de una sesion activa
        // para poder probar --info y --stop.
        if (setupActiveSession) {
            BorneoConfig config;
            config.eslora = 90.0;
            config.grilletes = 2;
            config.profundidad = 25.0;

            BorneoService service(&ctx);

            const BorneoOperationResult setupResult =
                service.startSession(config);

            QVERIFY2(
                setupResult.success,
                qPrintable(
                    QString(
                        "No se pudo preparar la sesion activa: %1"
                    ).arg(setupResult.message)
                )
            );

            QVERIFY(ctx.borneoSession.active);
        }

        BorneoCommand command;

        // ─────────────────────────────────────────────────────────
        // ACT
        // ─────────────────────────────────────────────────────────

        const CommandResult result =
            command.execute(invocation, ctx);

        // ─────────────────────────────────────────────────────────
        // ASSERT
        // ─────────────────────────────────────────────────────────

        QCOMPARE(
            result.ok,
            expectedSuccess
        );

        const bool containsMessage =
            result.message.contains(
                expectedMessageContains,
                Qt::CaseInsensitive
            );

        if (!containsMessage) {
            qDebug()
                << "\nCOMANDO:"
                << commandLine;

            qDebug()
                << "MENSAJE ESPERADO:"
                << expectedMessageContains;

            qDebug()
                << "MENSAJE OBTENIDO:"
                << result.message
                << "\n";
        }

        QVERIFY2(
            containsMessage,
            "El mensaje devuelto por BorneoCommand "
            "no contiene el texto esperado."
        );
    }
};

QTEST_APPLESS_MAIN(TestBorneoCommand)

#include "test_borneocommand.moc"