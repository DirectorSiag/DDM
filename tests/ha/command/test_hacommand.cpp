#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "haCommand.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"

class TestHaCommand : public QObject {
    Q_OBJECT

private slots:

    void testComandosJson_data() {
        QTest::addColumn<QString>("commandLine");
        QTest::addColumn<bool>("expectedSuccess");
        QTest::addColumn<QString>("expectedMessageContains");

        QFile file(":/json/json/hacommand_cases.json");
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Ruta:" << file.fileName() << "Error:" << file.errorString();
            QFAIL("No se pudo abrir el archivo JSON de pruebas. Revisa el .qrc");
        }

        QJsonArray cases = QJsonDocument::fromJson(file.readAll())
                               .object()["haCommandTestCases"].toArray();

        for (const QJsonValue& val : cases) {
            QJsonObject obj = val.toObject();
            QTest::newRow(obj["id"].toString().toStdString().c_str())
                << obj["commandLine"].toString()
                << obj["expectedSuccess"].toBool()
                << obj["expectedMessageContains"].toString();
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
        QVERIFY2(parseOk, qPrintable("CommandParser fallo: " + parseError));

        CommandContext ctx;
        HaCommand cmd;

        // ACT
        CommandResult res = cmd.execute(inv, ctx);

        // ASSERT
        QCOMPARE(res.ok, expectedSuccess);
        bool containsMsg = res.message.contains(expectedMessageContains, Qt::CaseInsensitive);
        if (!containsMsg) {
            qDebug() << "\nMENSAJE ESPERADO :" << expectedMessageContains;
            qDebug() << "MENSAJE OBTENIDO :" << res.message;
        }
        QVERIFY2(containsMsg, "El mensaje no contiene el texto esperado.");
    }
};

QTEST_APPLESS_MAIN(TestHaCommand)
#include "test_hacommand.moc"
