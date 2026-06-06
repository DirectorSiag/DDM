#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTest>

#include "commandContext.h"
#include "json/jsoncommandhandler.h"
#include "network/iTransport.h"

class FakeTransport : public ITransport
{
    Q_OBJECT

public:
    using ITransport::ITransport;

    bool send(const QByteArray& data) override
    {
        sentMessages.append(data);
        return true;
    }

    QList<QByteArray> sentMessages;
};

class TestJsonCommandHandler : public QObject
{
    Q_OBJECT

private slots:
    void processJsonCommand_responde_error_data();
    void processJsonCommand_responde_error();
    void processJsonCommand_rutea_create_line_y_responde_exito();
    void processJsonCommand_rutea_delete_line_y_responde_exito();
};

void TestJsonCommandHandler::processJsonCommand_responde_error_data()
{
    QTest::addColumn<QByteArray>("request");
    QTest::addColumn<QString>("expectedCommand");
    QTest::addColumn<QString>("expectedCode");
    QTest::addColumn<QString>("expectedMessage");
    QTest::addColumn<QString>("expectedMessagePrefix");
    QTest::addColumn<QString>("expectedDetailsField");

    QTest::newRow("comando desconocido")
        << QByteArray(R"({"command":"unsupported","args":{}})")
        << QString("unsupported")
        << QString("UNKNOWN_COMMAND")
        << QString("Comando no reconocido: unsupported")
        << QString()
        << QString();

    QTest::newRow("json malformado")
        << QByteArray("{invalid}")
        << QString("unknown")
        << QString("INVALID_JSON")
        << QString()
        << QString("Error al parsear JSON:")
        << QString();

    QTest::newRow("json no objeto")
        << QByteArray("[]")
        << QString("unknown")
        << QString("INVALID_JSON")
        << QString("Error al parsear JSON: El JSON debe ser un objeto")
        << QString()
        << QString();

    QTest::newRow("comando ausente")
        << QByteArray(R"({"args":{}})")
        << QString()
        << QString("UNKNOWN_COMMAND")
        << QString("Comando no reconocido: ")
        << QString()
        << QString();

    QTest::newRow("create_line sin args")
        << QByteArray(R"({"command":"create_line"})")
        << QString("create_line")
        << QString("INVALID_AZIMUT")
        << QString()
        << QString()
        << QString("azimut");

    QTest::newRow("create_line con args vacio")
        << QByteArray(R"({"command":"create_line","args":{}})")
        << QString("create_line")
        << QString("INVALID_AZIMUT")
        << QString()
        << QString()
        << QString("azimut");

    QTest::newRow("create_line con args no objeto")
        << QByteArray(R"({"command":"create_line","args":[]})")
        << QString("create_line")
        << QString("INVALID_AZIMUT")
        << QString()
        << QString()
        << QString("azimut");

    QTest::newRow("delete_line sin args")
        << QByteArray(R"({"command":"delete_line"})")
        << QString("delete_line")
        << QString("MISSING_REQUIRED_FIELDS")
        << QString()
        << QString()
        << QString();

    QTest::newRow("delete_line con args vacio")
        << QByteArray(R"({"command":"delete_line","args":{}})")
        << QString("delete_line")
        << QString("MISSING_REQUIRED_FIELDS")
        << QString()
        << QString()
        << QString();

    QTest::newRow("delete_line con args no objeto")
        << QByteArray(R"({"command":"delete_line","args":[]})")
        << QString("delete_line")
        << QString("MISSING_REQUIRED_FIELDS")
        << QString()
        << QString()
        << QString();
}

void TestJsonCommandHandler::processJsonCommand_responde_error()
{
    QFETCH(QByteArray, request);
    QFETCH(QString, expectedCommand);
    QFETCH(QString, expectedCode);
    QFETCH(QString, expectedMessage);
    QFETCH(QString, expectedMessagePrefix);
    QFETCH(QString, expectedDetailsField);

    FakeTransport transport;
    CommandContext context;
    JsonCommandHandler handler(&context, &transport);

    handler.processJsonCommand(request);

    QCOMPARE(transport.sentMessages.count(), 1);

    QJsonParseError parseError;
    const QJsonDocument responseDoc =
        QJsonDocument::fromJson(transport.sentMessages.first(), &parseError);

    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(responseDoc.isObject());

    const QJsonObject response = responseDoc.object();
    const QJsonObject args = response.value("args").toObject();

    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("command").toString(), expectedCommand);
    QCOMPARE(args.value("code").toString(), expectedCode);

    if (!expectedMessage.isEmpty()) {
        QCOMPARE(args.value("message").toString(), expectedMessage);
    }

    if (!expectedMessagePrefix.isEmpty()) {
        QVERIFY(args.value("message").toString().startsWith(expectedMessagePrefix));
    }

    if (!expectedDetailsField.isEmpty()) {
        const QJsonObject details = args.value("details").toObject();
        QCOMPARE(details.value("field").toString(), expectedDetailsField);
    }

    QCOMPARE(context.getCursors().size(), std::size_t(0));
}

void TestJsonCommandHandler::processJsonCommand_rutea_create_line_y_responde_exito()
{
    FakeTransport transport;
    CommandContext context;
    JsonCommandHandler handler(&context, &transport);

    handler.processJsonCommand(
        R"({"command":"create_line","args":{"azimut":90,"length":15,"type":3,"x":10,"y":20}})"
    );

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(context.getCursors().size(), std::size_t(1));

    QJsonParseError parseError;
    const QJsonDocument responseDoc =
        QJsonDocument::fromJson(transport.sentMessages.first(), &parseError);

    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(responseDoc.isObject());

    const QJsonObject response = responseDoc.object();
    const QJsonObject args = response.value("args").toObject();
    const QJsonArray lines = args.value("lines").toArray();
    const QJsonObject line = lines.first().toObject();

    QCOMPARE(response.value("status").toString(), QString("success"));
    QCOMPARE(response.value("command").toString(), QString("create_line"));
    QCOMPARE(args.value("created_id").toString(), QString("LINE_2"));
    QCOMPARE(lines.size(), 1);
    QCOMPARE(line.value("id").toString(), QString("LINE_2"));
}

void TestJsonCommandHandler::processJsonCommand_rutea_delete_line_y_responde_exito()
{
    FakeTransport transport;
    CommandContext context;
    JsonCommandHandler handler(&context, &transport);

    handler.processJsonCommand(
        R"({"command":"create_line","args":{"azimut":90,"length":15}})"
    );

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(context.getCursors().size(), std::size_t(1));

    transport.sentMessages.clear();

    handler.processJsonCommand(R"({"command":"delete_line","args":{"id":"LINE_2"}})");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(context.getCursors().size(), std::size_t(0));

    QJsonParseError parseError;
    const QJsonDocument responseDoc =
        QJsonDocument::fromJson(transport.sentMessages.first(), &parseError);

    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(responseDoc.isObject());

    const QJsonObject response = responseDoc.object();
    const QJsonObject args = response.value("args").toObject();
    const QJsonArray lines = args.value("lines").toArray();

    QCOMPARE(response.value("status").toString(), QString("success"));
    QCOMPARE(response.value("command").toString(), QString("delete_line"));
    QCOMPARE(args.value("deleted_id").toString(), QString("LINE_2"));
    QCOMPARE(lines.size(), 0);
}

QTEST_APPLESS_MAIN(TestJsonCommandHandler)

#include "tst_jsoncommandhandler.moc"
