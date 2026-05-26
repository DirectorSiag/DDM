#include <QSignalSpy>
#include <QTest>

#include "concDecoder.h"

class TestDecoder : public QObject
{
    Q_OBJECT

private slots:
    void decode_emits_newRange_when_range_bits_change();
};

void TestDecoder::decode_emits_newRange_when_range_bits_change()
{
    ConcDecoder decoder;
    QSignalSpy spy(&decoder, &ConcDecoder::newRange);

    QByteArray message(27, 0);
    message[0] = static_cast<char>(0x00);

    decoder.decode(message);

    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 2);
}

QTEST_APPLESS_MAIN(TestDecoder)

#include "tst_decoder.moc"