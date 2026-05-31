#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "commandContext.h"
#include "concDecoder.h"
#include "dclConcController.h"
#include "json/jsoncommandhandler.h"
#include "messagerouter.h"
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

class RecordingDecoder : public ConcDecoder
{
public:
    void decode(const QByteArray& message) override
    {
        decodedMessages.append(message);
    }

    QList<QByteArray> decodedMessages;
};

class TestMessageRouter : public QObject
{
    Q_OBJECT

private slots:
    void onMessageReceived_rutea_datagrama_binario_al_controlador_dcl();
    void onMessageReceived_rutea_json_al_handler_json();
    void onMessageReceived_rutea_json_malformado_al_handler_json();
    void onMessageReceived_ignora_datagrama_vacio();
    void onMessageReceived_ignora_datagrama_binario_menor_a_tres_bytes();
    void onMessageReceived_acepta_json_con_espacios_exteriores();
    void onMessageReceived_acepta_json_con_salto_de_linea_final();
    void onMessageReceived_rutea_json_sin_llave_de_cierre_al_handler_json();
    void onMessageReceived_rutea_array_json_al_handler_json();
};

void TestMessageRouter::onMessageReceived_rutea_datagrama_binario_al_controlador_dcl()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(QByteArray::fromHex("00123400FFAA55"));

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(transport.sentMessages.first(), QByteArray::fromHex("049234"));
    QCOMPARE(decoder.decodedMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.first(), QByteArray::fromHex("FF0055AA"));
}

void TestMessageRouter::onMessageReceived_rutea_json_al_handler_json()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(R"({"command":"unsupported","args":{}})");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("command").toString(), QString("unsupported"));
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("UNKNOWN_COMMAND"));
}

void TestMessageRouter::onMessageReceived_rutea_json_malformado_al_handler_json()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived("{invalid}");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("INVALID_JSON"));
}

void TestMessageRouter::onMessageReceived_ignora_datagrama_vacio()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(QByteArray());

    QCOMPARE(transport.sentMessages.count(), 0);
    QCOMPARE(decoder.decodedMessages.count(), 0);
}

void TestMessageRouter::onMessageReceived_ignora_datagrama_binario_menor_a_tres_bytes()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(QByteArray::fromHex("0102"));

    QCOMPARE(transport.sentMessages.count(), 0);
    QCOMPARE(decoder.decodedMessages.count(), 0);
}

void TestMessageRouter::onMessageReceived_acepta_json_con_espacios_exteriores()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(R"(  {"command":"unsupported","args":{}}  )");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("UNKNOWN_COMMAND"));
}

void TestMessageRouter::onMessageReceived_acepta_json_con_salto_de_linea_final()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived("{\"command\":\"unsupported\",\"args\":{}}\n");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("UNKNOWN_COMMAND"));
}

void TestMessageRouter::onMessageReceived_rutea_json_sin_llave_de_cierre_al_handler_json()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived(R"({"command":"unsupported","args":{})");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("INVALID_JSON"));
}

void TestMessageRouter::onMessageReceived_rutea_array_json_al_handler_json()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController dclController(&transport, &decoder);
    CommandContext context;
    JsonCommandHandler jsonHandler(&context, &transport);
    MessageRouter router(&dclController, &jsonHandler);

    router.onMessageReceived("[]");

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.count(), 0);

    const QJsonObject response =
        QJsonDocument::fromJson(transport.sentMessages.first()).object();
    QCOMPARE(response.value("args").toObject().value("code").toString(),
             QString("INVALID_JSON"));
}

QTEST_APPLESS_MAIN(TestMessageRouter)

#include "tst_messagerouter.moc"
