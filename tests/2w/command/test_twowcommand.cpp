#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include "TwoWCommand.h"
#include "commandContext.h"
#include "iCommand.h"
#include "view/CommandParser.h"

class TestTwoWCommand : public QObject {
    Q_OBJECT

private slots:
    void testComandosJson_data() {
        QTest::addColumn<QString>("commandLine");
        QTest::addColumn<bool>("expectedSuccess");
        QTest::addColumn<QString>("expectedMessageContains");

        QFile file(":/json/json/twowcommand_cases.json");
        if (!file.open(QIODevice::ReadOnly)) {
            QFAIL("No se pudo abrir el archivo JSON de pruebas. Revisa el .qrc");
        }

        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray cases = doc.object()["twoWCommandTestCases"].toArray();

        // Arma las filas de prueba dinámicamente
        for (const QJsonValue& val : cases) {
            QJsonObject obj = val.toObject();
            QString id = obj["id"].toString();
            QString cmdLine = obj["commandLine"].toString();
            bool success = obj["expectedSuccess"].toBool();
            QString msg = obj["expectedMessageContains"].toString();

            QTest::newRow(id.toStdString().c_str()) << cmdLine << success << msg;
        }
    }

    // 2. FASE DE EJECUCIÓN (Corre una vez por cada fila del JSON)
    void testComandosJson() {
        // Obtenemos los datos de la fila actual
        QFETCH(QString, commandLine);
        QFETCH(bool, expectedSuccess);
        QFETCH(QString, expectedMessageContains);

        // ARRANGE (Preparar el entorno)
        CommandParser parser;
        CommandInvocation inv;
        QString parseError;

        bool parseOk = parser.parse(commandLine, inv, parseError);
        QVERIFY2(parseOk, qPrintable("El CommandParser fallo leyendo el JSON: " + parseError));

        CommandContext ctx;
        TwoWCommand cmd;

        // ACT (Ejecutar el comando)
        CommandResult res = cmd.execute(inv, ctx);

        // ASSERT (Verificar resultados)
        QCOMPARE(res.ok, expectedSuccess);

        bool containsMsg = res.message.contains(expectedMessageContains, Qt::CaseInsensitive);
        if (!containsMsg) {
            qDebug() << "\nMENSAJE ESPERADO :" << expectedMessageContains;
            qDebug() << "MENSAJE OBTENIDO :" << res.message << "\n";
        }
        QVERIFY2(containsMsg, "El mensaje devuelto por el comando no contiene el texto esperado.");
    }
};

QTEST_APPLESS_MAIN(TestTwoWCommand)
#include "test_twowcommand.moc"