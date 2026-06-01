#include <QSignalSpy>
#include <QTest>

#include "concDecoder.h"
#include "dclConcController.h"
#include "iTransport.h"

namespace {

QByteArray buildDclConcDatagram(quint16 sequence,
                                const QByteArray& payload = QByteArray(27, char(0x00)))
{
    QByteArray datagram;
    datagram.reserve(3 + payload.size());
    datagram.append(char(0x04));
    datagram.append(char((sequence >> 8) & 0x7F));
    datagram.append(char(sequence & 0xFF));
    datagram.append(payload);
    return datagram;
}

}

class FakeTransport : public ITransport
{
    Q_OBJECT

public:
    using ITransport::ITransport;

    bool send(const QByteArray& data) override
    {
        if (eventLog) {
            eventLog->append("send");
        }
        sentMessages.append(data);
        return true;
    }

    QList<QByteArray> sentMessages;
    QList<QString>* eventLog = nullptr;
};

class RecordingDecoder : public ConcDecoder
{
public:
    void decode(const QByteArray& message) override
    {
        if (eventLog) {
            eventLog->append("decode");
        }
        decodedMessages.append(message);
    }

    QList<QByteArray> decodedMessages;
    QList<QString>* eventLog = nullptr;
};

class TestDclConcController : public QObject
{
    Q_OBJECT

private slots:
    void negateData_invierte_todos_los_bytes_data();
    void negateData_invierte_todos_los_bytes();
    void onDatagram_ignora_datagrama_menor_a_tres_bytes_data();
    void onDatagram_ignora_datagrama_menor_a_tres_bytes();
    void onDatagram_ignora_mensaje_and1();
    void onDatagram_ignora_ack();
    void onDatagram_envia_ack_con_secuencia_correcta_data();
    void onDatagram_envia_ack_con_secuencia_correcta();
    void onDatagram_ignora_datagrama_con_solo_header();
    void onDatagram_con_payload_entrega_al_decoder_el_payload_invertido();
    void onDatagram_envia_ack_antes_de_decodificar_payload();
};

void TestDclConcController::negateData_invierte_todos_los_bytes_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("patron mixto") << QByteArray::fromHex("00FFAA55")
                                   << QByteArray::fromHex("FF0055AA");
    QTest::newRow("todo cero") << QByteArray::fromHex("000000")
                                << QByteArray::fromHex("FFFFFF");
    QTest::newRow("todo uno") << QByteArray::fromHex("FFFFFF")
                               << QByteArray::fromHex("000000");
}

void TestDclConcController::negateData_invierte_todos_los_bytes()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expected);

    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    QCOMPARE(controller.negateData(input), expected);
}

void TestDclConcController::onDatagram_ignora_datagrama_menor_a_tres_bytes_data()
{
    QTest::addColumn<QByteArray>("datagram");

    QTest::newRow("vacio") << QByteArray();
    QTest::newRow("un byte") << QByteArray::fromHex("00");
    QTest::newRow("dos bytes") << QByteArray::fromHex("0000");
}

void TestDclConcController::onDatagram_ignora_datagrama_menor_a_tres_bytes()
{
    QFETCH(QByteArray, datagram);

    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 0);
}

void TestDclConcController::onDatagram_ignora_mensaje_and1()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController controller(&transport, &decoder);

    // AND1 usa Device Address 0x02 y una longitud total de 51 bytes.
    const QByteArray datagram = QByteArray::fromHex("020001")
                                + QByteArray(48, char(0x00));

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 0);
    QCOMPARE(decoder.decodedMessages.count(), 0);
}

void TestDclConcController::onDatagram_ignora_ack()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController controller(&transport, &decoder);

    const QByteArray datagram = QByteArray::fromHex("048001");

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 0);
    QCOMPARE(decoder.decodedMessages.count(), 0);
}

void TestDclConcController::onDatagram_envia_ack_con_secuencia_correcta_data()
{
    QTest::addColumn<QByteArray>("datagram");
    QTest::addColumn<QByteArray>("expectedAck");

    QTest::newRow("secuencia media") << buildDclConcDatagram(0x1234)
                                      << QByteArray::fromHex("049234");
    QTest::newRow("secuencia cero") << buildDclConcDatagram(0x0000)
                                     << QByteArray::fromHex("048000");
    QTest::newRow("secuencia maxima") << buildDclConcDatagram(0x7FFF)
                                       << QByteArray::fromHex("04FFFF");
}

void TestDclConcController::onDatagram_envia_ack_con_secuencia_correcta()
{
    QFETCH(QByteArray, datagram);
    QFETCH(QByteArray, expectedAck);

    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController controller(&transport, &decoder);

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(transport.sentMessages.first(), expectedAck);
}

void TestDclConcController::onDatagram_ignora_datagrama_con_solo_header()
{
    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);
    QSignalSpy obmSpy(&decoder, &ConcDecoder::signalOBM);
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);
    QSignalSpy handWheelSpy(&decoder, &ConcDecoder::newHandWheel);
    QSignalSpy rollingBallSpy(&decoder, &ConcDecoder::newRollingBall);
    QSignalSpy centLeftSpy(&decoder, &ConcDecoder::centLeft);
    QSignalSpy resetObmLeftSpy(&decoder, &ConcDecoder::resetObmLeft);
    QSignalSpy dataReqLeftSpy(&decoder, &ConcDecoder::dataReqLeft);
    QSignalSpy cuOrOffCentLeftSpy(&decoder, &ConcDecoder::cuOrOffCentLeft);
    QSignalSpy cuOrCentLeftSpy(&decoder, &ConcDecoder::cuOrCentLeft);
    QSignalSpy offCentLeftSpy(&decoder, &ConcDecoder::offCentLeft);
    QSignalSpy trueMotionSpy(&decoder, &ConcDecoder::trueMotion);
    QSignalSpy ownCursSpy(&decoder, &ConcDecoder::ownCurs);

    const QByteArray datagram = QByteArray::fromHex("040001");

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 0);
    QCOMPARE(obmSpy.count(), 0);
    QCOMPARE(rangeSpy.count(), 0);
    QCOMPARE(qekSpy.count(), 0);
    QCOMPARE(overlaySpy.count(), 0);
    QCOMPARE(handWheelSpy.count(), 0);
    QCOMPARE(rollingBallSpy.count(), 0);
    QCOMPARE(centLeftSpy.count(), 0);
    QCOMPARE(resetObmLeftSpy.count(), 0);
    QCOMPARE(dataReqLeftSpy.count(), 0);
    QCOMPARE(cuOrOffCentLeftSpy.count(), 0);
    QCOMPARE(cuOrCentLeftSpy.count(), 0);
    QCOMPARE(offCentLeftSpy.count(), 0);
    QCOMPARE(trueMotionSpy.count(), 0);
    QCOMPARE(ownCursSpy.count(), 0);
}

void TestDclConcController::onDatagram_con_payload_entrega_al_decoder_el_payload_invertido()
{
    FakeTransport transport;
    RecordingDecoder decoder;
    DclConcController controller(&transport, &decoder);

    const QByteArray payload = QByteArray::fromHex("00FFAA55")
                               + QByteArray(23, char(0x00));
    const QByteArray datagram = buildDclConcDatagram(0x1234, payload);
    const QByteArray expectedAck = QByteArray::fromHex("049234");
    const QByteArray expectedPayload = QByteArray::fromHex("FF0055AA")
                                       + QByteArray(23, char(0xFF));

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(transport.sentMessages.first(), expectedAck);
    QCOMPARE(decoder.decodedMessages.count(), 1);
    QCOMPARE(decoder.decodedMessages.first(), expectedPayload);
}

void TestDclConcController::onDatagram_envia_ack_antes_de_decodificar_payload()
{
    QList<QString> eventLog;
    FakeTransport transport;
    transport.eventLog = &eventLog;
    RecordingDecoder decoder;
    decoder.eventLog = &eventLog;
    DclConcController controller(&transport, &decoder);

    const QByteArray datagram = buildDclConcDatagram(0x1234);

    controller.onDatagram(datagram);

    QCOMPARE(eventLog.count(), 2);
    QCOMPARE(eventLog.at(0), QString("send"));
    QCOMPARE(eventLog.at(1), QString("decode"));
}

QTEST_MAIN(TestDclConcController)

#include "tst_dclconccontroller.moc"
