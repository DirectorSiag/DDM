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
        sentMessages.append(data);
        return true;
    }

    QList<QByteArray> sentMessages;
};

class TestDclConcController : public QObject
{
    Q_OBJECT

private slots:
    void negateData_invierte_todos_los_bytes();
    void onDatagram_ignora_datagrama_menor_a_tres_bytes();
    void onDatagram_envia_ack_con_secuencia_correcta();
    void onDatagram_con_solo_header_envia_ack_y_no_emite_eventos_del_decoder();
};

void TestDclConcController::negateData_invierte_todos_los_bytes()
{
    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    const QByteArray input = QByteArray::fromHex("00FFAA55");
    const QByteArray expected = QByteArray::fromHex("FF0055AA");

    QCOMPARE(controller.negateData(input), expected);
}

void TestDclConcController::onDatagram_ignora_datagrama_menor_a_tres_bytes()
{
    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    controller.onDatagram(QByteArray());
    controller.onDatagram(QByteArray(1, char(0x00)));
    controller.onDatagram(QByteArray(2, char(0x00)));

    QCOMPARE(transport.sentMessages.count(), 0);
}

void TestDclConcController::onDatagram_envia_ack_con_secuencia_correcta()
{
    FakeTransport transport;
    ConcDecoder decoder;
    DclConcController controller(&transport, &decoder);

    const QByteArray datagram = QByteArray::fromHex("001234");
    const QByteArray expectedAck = QByteArray::fromHex("049234");

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

QTEST_MAIN(TestDclConcController)

#include "tst_dclconccontroller.moc"
