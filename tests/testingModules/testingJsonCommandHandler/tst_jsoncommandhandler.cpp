#include <QJsonDocument>
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
    void processJsonCommand_responde_error_para_comando_desconocido();
};

void TestJsonCommandHandler::processJsonCommand_responde_error_para_comando_desconocido()
{
    FakeTransport transport;
    CommandContext context;
    JsonCommandHandler handler(&context, &transport);

    handler.processJsonCommand(R"({"command":"unsupported","args":{}})");

    QCOMPARE(transport.sentMessages.count(), 1);

    QJsonParseError parseError;
    const QJsonDocument responseDoc =
        QJsonDocument::fromJson(transport.sentMessages.first(), &parseError);

    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(responseDoc.isObject());

    const QJsonObject response = responseDoc.object();
    const QJsonObject args = response.value("args").toObject();

    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("command").toString(), QString("unsupported"));
    QCOMPARE(args.value("code").toString(), QString("UNKNOWN_COMMAND"));
    QCOMPARE(args.value("message").toString(),
             QString("Comando no reconocido: unsupported"));
    QCOMPARE(context.getCursors().size(), std::size_t(0));
}

QTEST_APPLESS_MAIN(TestJsonCommandHandler)

#include "tst_jsoncommandhandler.moc"
