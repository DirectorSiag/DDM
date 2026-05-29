#include <QSignalSpy>
#include <QTest>

#include "concDecoder.h"
#include "dclConcController.h"
#include "iTransport.h"

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
    void onDatagram_envia_ack_con_secuencia_correcta_data();
    void onDatagram_envia_ack_con_secuencia_correcta();
    void onDatagram_con_solo_header_envia_ack_y_no_emite_eventos_del_decoder();
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

void TestDclConcController::onDatagram_envia_ack_con_secuencia_correcta_data()
{
    QTest::addColumn<QByteArray>("datagram");
    QTest::addColumn<QByteArray>("expectedAck");

    QTest::newRow("secuencia media") << QByteArray::fromHex("001234")
                                      << QByteArray::fromHex("049234");
    QTest::newRow("secuencia cero") << QByteArray::fromHex("000000")
                                     << QByteArray::fromHex("048000");
    QTest::newRow("secuencia maxima") << QByteArray::fromHex("007FFF")
                                       << QByteArray::fromHex("04FFFF");
    QTest::newRow("mascara bit alto") << QByteArray::fromHex("00FFFF")
                                       << QByteArray::fromHex("04FFFF");
}

void TestDclConcController::onDatagram_envia_ack_con_secuencia_correcta()
{
    QFETCH(QByteArray, datagram);
    QFETCH(QByteArray, expectedAck);

    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(transport.sentMessages.first(), expectedAck);
}

void TestDclConcController::onDatagram_con_solo_header_envia_ack_y_no_emite_eventos_del_decoder()
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

    const QByteArray datagram = QByteArray::fromHex("000001");
    const QByteArray expectedAck = QByteArray::fromHex("048001");

    controller.onDatagram(datagram);

    QCOMPARE(transport.sentMessages.count(), 1);
    QCOMPARE(transport.sentMessages.first(), expectedAck);
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

    const QByteArray datagram = QByteArray::fromHex("00123400FFAA55");
    const QByteArray expectedAck = QByteArray::fromHex("049234");
    const QByteArray expectedPayload = QByteArray::fromHex("FF0055AA");

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

    const QByteArray datagram = QByteArray::fromHex("00123400FFAA55");

    controller.onDatagram(datagram);

    QCOMPARE(eventLog.count(), 2);
    QCOMPARE(eventLog.at(0), QString("send"));
    QCOMPARE(eventLog.at(1), QString("decode"));
}

QTEST_MAIN(TestDclConcController)

#include "tst_dclconccontroller.moc"
