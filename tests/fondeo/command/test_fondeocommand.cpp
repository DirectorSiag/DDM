#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "fondeoCommand.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"

class TestFondeoCommand : public QObject {
    Q_OBJECT
private slots:
    void testComandosJson_data() {
        QTest::addColumn<QString>("commandLine");
        QTest::addColumn<bool>("expectedSuccess");
        QTest::addColumn<QString>("expectedMessageContains");

        QFile file(":/jsons/json/fondeocommand_cases.json");
        if (!file.open(QIODevice::ReadOnly)) {
            QFAIL("No se pudo abrir el archivo JSON de pruebas. Revisa el .qrc");
        }

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray cases = doc.object()["fondeoCommandTestCases"].toArray();

        for (const QJsonValue& val : cases) {
            QJsonObject obj = val.toObject();
            QString id      = obj["id"].toString();
            QString cmdLine = obj["commandLine"].toString();
            bool success    = obj["expectedSuccess"].toBool();
            QString msg     = obj["expectedMessageContains"].toString();
            QTest::newRow(id.toStdString().c_str()) << cmdLine << success << msg;
        }
    }

    void testComandosJson() {
        QFETCH(QString, commandLine);
        QFETCH(bool, expectedSuccess);
        QFETCH(QString, expectedMessageContains);

        // ARRANGE
        CommandParser parser;
        CommandInvocation inv;
        QString parseError;
        bool parseOk = parser.parse(commandLine, inv, parseError);
        QVERIFY2(parseOk, qPrintable("El CommandParser fallo leyendo el JSON: " + parseError));

        CommandContext ctx;

        // Para CMD_01: necesitamos que el track 1 exista en el contexto
        if (commandLine.contains("--track=1")) {
            Track t;
            t.setId(1);
            t.setX(5.0f);
            t.setY(3.0f);
            ctx.tracks.push_back(t);
        }

        FondeoCommand cmd;

        // ACT
        CommandResult res = cmd.execute(inv, ctx);

        // ASSERT
        QCOMPARE(res.ok, expectedSuccess);
        bool containsMsg = res.message.contains(expectedMessageContains, Qt::CaseInsensitive);
        if (!containsMsg) {
            qDebug() << "\nMENSAJE ESPERADO :" << expectedMessageContains;
            qDebug() << "MENSAJE OBTENIDO :" << res.message << "\n";
        }
        QVERIFY2(containsMsg, "El mensaje devuelto por el comando no contiene el texto esperado.");
    }
};

QTEST_APPLESS_MAIN(TestFondeoCommand)
#include "test_fondeocommand.moc"