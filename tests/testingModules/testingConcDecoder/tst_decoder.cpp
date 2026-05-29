#include <QSignalSpy>
#include <QTest>

#include "concDecoder.h"


static QByteArray buildBaselineFullFrame()
{
    QByteArray frame(27, 0);

    // Word 1: range 000 -> RANGE_2_DM
    frame[0] = static_cast<char>(0x00);

    // Word 2: raise all implemented left-side control bits + own cursor
    // bits 0..6 => cuOrOffCentLeft, cuOrCentLeft, offCentLeft, centLeft,
    //              resetObmLeft, dataReqLeft, trueMotion
    // bit 7 => ownCurs(true)
    frame[3] = static_cast<char>(0xFF);

    // Word 3: TODO (decodeWord3 is still commented out in ConcDecoder::decode())

    // Word 4: QEK_20 -> 00010000
    frame[9] = static_cast<char>(0x10);
    frame[10] = static_cast<char>(0x00); // slave side stays at QEK_NONE

    // Word 5: overlay master 0001 -> SPC
    frame[12] = static_cast<char>(0x01);

    // Word 6: handwheel raw bytes -> current implementation yields (0, 0)
    frame[15] = static_cast<char>(0x00);
    frame[16] = static_cast<char>(0x00);

    // Word 7: rolling ball raw bytes -> dx = -2, dy = 3
    frame[18] = static_cast<char>(0xFE);
    frame[19] = static_cast<char>(0x03);

    // Word 8: TODO (decodeWord8 exists but is not called from ConcDecoder::decode())

    return frame;
}

class TestDecoder : public QObject
{
    Q_OBJECT

private slots:
    void decode_full_frame_emits_range_data();
    void decode_full_frame_emits_range();

    void decode_full_frame_emits_word2_control_data();
    void decode_full_frame_emits_word2_control();

    void decode_full_frame_emits_qek_data();
    void decode_full_frame_emits_qek();

    void decode_full_frame_emits_overlay_data();
    void decode_full_frame_emits_overlay();

    void decode_full_frame_emits_handwheel_data();
    void decode_full_frame_emits_handwheel();

    void decode_full_frame_emits_rolling_data();
    void decode_full_frame_emits_rolling();
};

void TestDecoder::decode_full_frame_emits_range_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");

    QTest::newRow("baseline full frame") << buildBaselineFullFrame() << 2;
}

void TestDecoder::decode_full_frame_emits_range()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedRange);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    const QList<QVariant> rangeArguments = rangeSpy.takeFirst();
    QCOMPARE(rangeArguments.at(0).toInt(), expectedRange);
}

void TestDecoder::decode_full_frame_emits_word2_control_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<bool>("expectedOwnCursor");

    QTest::newRow("baseline full frame") << buildBaselineFullFrame() << true;
}

void TestDecoder::decode_full_frame_emits_word2_control()
{
    QFETCH(QByteArray, frame);
    QFETCH(bool, expectedOwnCursor);

    ConcDecoder decoder;

    QSignalSpy cuOrOffCentLeftSpy(&decoder, &ConcDecoder::cuOrOffCentLeft);
    QSignalSpy cuOrCentLeftSpy(&decoder, &ConcDecoder::cuOrCentLeft);
    QSignalSpy offCentLeftSpy(&decoder, &ConcDecoder::offCentLeft);
    QSignalSpy centLeftSpy(&decoder, &ConcDecoder::centLeft);
    QSignalSpy resetObmLeftSpy(&decoder, &ConcDecoder::resetObmLeft);
    QSignalSpy dataReqLeftSpy(&decoder, &ConcDecoder::dataReqLeft);
    QSignalSpy trueMotionSpy(&decoder, &ConcDecoder::trueMotion);
    QSignalSpy ownCursorSpy(&decoder, &ConcDecoder::ownCurs);

    decoder.decode(frame);

    QCOMPARE(cuOrOffCentLeftSpy.count(), 1);
    QCOMPARE(cuOrCentLeftSpy.count(), 1);
    QCOMPARE(offCentLeftSpy.count(), 1);
    QCOMPARE(centLeftSpy.count(), 1);
    QCOMPARE(resetObmLeftSpy.count(), 1);
    QCOMPARE(dataReqLeftSpy.count(), 1);
    QCOMPARE(trueMotionSpy.count(), 1);
    QCOMPARE(ownCursorSpy.count(), 1);

    const QList<QVariant> ownCursorArguments = ownCursorSpy.takeFirst();
    QCOMPARE(ownCursorArguments.at(0).toBool(), expectedOwnCursor);
}

void TestDecoder::decode_full_frame_emits_qek_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<QString>("expectedQek");

    QTest::newRow("baseline full frame")
        << buildBaselineFullFrame()
        << QStringLiteral("QEK_20");
}

void TestDecoder::decode_full_frame_emits_qek()
{
    QFETCH(QByteArray, frame);
    QFETCH(QString, expectedQek);

    ConcDecoder decoder;
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    decoder.decode(frame);

    QCOMPARE(qekSpy.count(), 1);
    const QList<QVariant> qekArguments = qekSpy.takeFirst();
    QCOMPARE(qekArguments.at(0).toString(), expectedQek);
}

void TestDecoder::decode_full_frame_emits_overlay_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<QString>("expectedOverlay");

    QTest::newRow("baseline full frame")
        << buildBaselineFullFrame()
        << QStringLiteral("SPC");
}

void TestDecoder::decode_full_frame_emits_overlay()
{
    QFETCH(QByteArray, frame);
    QFETCH(QString, expectedOverlay);

    ConcDecoder decoder;
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);

    decoder.decode(frame);

    QCOMPARE(overlaySpy.count(), 1);
    const QList<QVariant> overlayArguments = overlaySpy.takeFirst();
    QCOMPARE(overlayArguments.at(0).toString(), expectedOverlay);
}

void TestDecoder::decode_full_frame_emits_handwheel_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<float>("expectedHandWheelPhi");
    QTest::addColumn<float>("expectedHandWheelRho");

    QTest::newRow("baseline full frame")
        << buildBaselineFullFrame()
        << 0.0f
        << 0.0f;
}

void TestDecoder::decode_full_frame_emits_handwheel()
{
    QFETCH(QByteArray, frame);
    QFETCH(float, expectedHandWheelPhi);
    QFETCH(float, expectedHandWheelRho);

    ConcDecoder decoder;

    int handWheelCalls = 0;
    QPair<float, float> handWheelValue;
    QObject::connect(&decoder,
                     &ConcDecoder::newHandWheel,
                     &decoder,
                     [&](QPair<float, float> value)
                     {
                         handWheelValue = value;
                         ++handWheelCalls;
                     });

    decoder.decode(frame);

    QCOMPARE(handWheelCalls, 1);
    QCOMPARE(handWheelValue.first, expectedHandWheelPhi);
    QCOMPARE(handWheelValue.second, expectedHandWheelRho);
}

void TestDecoder::decode_full_frame_emits_rolling_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRollingDx");
    QTest::addColumn<int>("expectedRollingDy");

    QTest::newRow("baseline full frame")
        << buildBaselineFullFrame()
        << -2
        << 3;
}

void TestDecoder::decode_full_frame_emits_rolling()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedRollingDx);
    QFETCH(int, expectedRollingDy);

    ConcDecoder decoder;

    int rollingCalls = 0;
    QPair<int, int> rollingValue;
    QObject::connect(&decoder,
                     &ConcDecoder::newRollingBall,
                     &decoder,
                     [&](QPair<int, int> value)
                     {
                         rollingValue = value;
                         ++rollingCalls;
                     });

    decoder.decode(frame);

    QCOMPARE(rollingCalls, 1);
    QCOMPARE(rollingValue.first, expectedRollingDx);
    QCOMPARE(rollingValue.second, expectedRollingDy);
}

QTEST_APPLESS_MAIN(TestDecoder)

#include "tst_decoder.moc"
