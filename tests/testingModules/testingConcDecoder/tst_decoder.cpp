#include <QSignalSpy>
#include <QTest>
#include <QPair>

#include <QVector>
#include <QString>
#include <QTime>
#include <QtGlobal>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QProcess>
#include <QDebug>

#include "concDecoder.h"

static QByteArray buildDynamicFrame(uint8_t rawWord1, uint8_t rawWord2, uint8_t qekByte, uint8_t overlayByte, int8_t dx, int8_t dy, int totalSize = 27)
{
    // Simula un datagrama UDP truncado o cortado por error de socket
    if (totalSize < 27)
    {
        return QByteArray(totalSize, 0);
    }

    QByteArray frame(27, 0);

    // ConcDecoder interpreta los bits tal como llegan (MSB-first), sin aplicar NOT.
    // Por eso escribimos el valor crudo del test directamente.
    frame[0] = static_cast<char>(rawWord1);
    frame[3] = static_cast<char>(rawWord2);

    // Palabra 4: Teclado de Entrada Rápida (QEK) del lado izquierdo
    frame[9] = static_cast<char>(qekByte);

    // Rellenamos la mitad derecha con valores numéricos determinísticos
    // Nota: por requerimiento, el decoder no decodificará estos valores,
    // pero los dejamos con contenido válido para robustez de parsing.
    uint8_t rightBase = static_cast<uint8_t>(rawWord1) ^ 0x5A;
    frame[10] = static_cast<char>(rightBase);
    frame[11] = static_cast<char>(rightBase + 1);
    frame[13] = static_cast<char>(rightBase + 2);
    frame[14] = static_cast<char>(rightBase + 3);

    // Palabra 5: Overlay Maestro (Izquierda)
    frame[12] = static_cast<char>(overlayByte);

    // Palabra 7: Rolling Ball Izquierda (Conversión directa a complemento a dos)
    frame[18] = static_cast<char>(dx);
    frame[19] = static_cast<char>(dy);

    // Rellenado adicional en la derecha: bytes 20..26 con secuencia
    for (int i = 20; i <= 26; ++i)
    {
        frame[i] = static_cast<char>(rightBase + (i - 20));
    }

    return frame;
}

static void setHandWheelBytes(QByteArray &frame, uint8_t phiByte, uint8_t rhoByte)
{
    if (frame.size() >= 17)
    {
        frame[15] = static_cast<char>(phiByte);
        frame[16] = static_cast<char>(rhoByte);
    }
}

class TestDecoder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void decode_frame_ranges_data();
    void decode_frame_ranges();

    void decode_rolling_signed_values_data();
    void decode_rolling_signed_values();

    void decode_truncated_frames_data();
    void decode_truncated_frames();

    void decode_qek_data();
    void decode_qek();

    void decode_handwheel_data();
    void decode_handwheel();

    void decode_overlay_data();
    void decode_overlay();

    void decode_multi_signal();

    void decode_malformed_header_data();
    void decode_malformed_header();

    void decode_word2_flags_data();
    void decode_word2_flags();

    void decode_word2_owncurs_transition();

    void decode_reserved_corrupt();

    void decode_range_limits_data();
    void decode_range_limits();

    void decode_burst_sequence();
    void decode_repeated_identical();
    void decode_mixed_valid_and_corrupt_sequence();

    void decode_endianness();

    void decode_signal_order();

    void decode_overlay_change();

    void decode_fuzzing_random();
};

void TestDecoder::initTestCase()
{
    qRegisterMetaType<QPair<float, float>>("QPair<float,float>");
}

void TestDecoder::decode_frame_ranges_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");

    // Formato de inyección: buildDynamicFrame(Word1, Word2, QEK, Overlay, dx, dy)
    // Word1 se interpreta por los 3 bits más significativos del primer byte.

    // Escenarios Nominales de Escala (Bits 24, 23, 22 del manual)
    QTest::newRow("Escala Minima - 2 DM") << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 0, 0) << 2;
    QTest::newRow("Escala Intermedia - 4 DM") << buildDynamicFrame(0x20, 0xFF, 0x10, 0x01, 0, 0) << 4;
    QTest::newRow("Escala Tactica - 16 DM") << buildDynamicFrame(0x60, 0xFF, 0x10, 0x01, 0, 0) << 16;
    QTest::newRow("Escala Maxima - 256 DM") << buildDynamicFrame(0xE0, 0xFF, 0x10, 0x01, 0, 0) << 256;

    // Escenarios de estrés: Ruido en la derecha e infiltración de comandos
    QTest::newRow("Escala 16 DM con datos basura en canal derecho/esclavo")
        << buildDynamicFrame(0x60, 0x00, 0x00, 0x00, -5, 12) << 16;

    QTest::newRow("Escala 2 DM con movimiento brusco simultáneo de Rolling Ball")
        << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 127, -128) << 2;
}

void TestDecoder::decode_frame_ranges()
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

void TestDecoder::decode_rolling_signed_values_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<float>("expectedDx");
    QTest::addColumn<float>("expectedDy");

    QTest::newRow("Rolling max/min signed")
        << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 127, -128)
        << 127.0f
        << -128.0f;

    QTest::newRow("Rolling negative/positive signed")
        << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, -2, 3)
        << -2.0f
        << 3.0f;
}

void TestDecoder::decode_rolling_signed_values()
{
    QFETCH(QByteArray, frame);
    QFETCH(float, expectedDx);
    QFETCH(float, expectedDy);

    ConcDecoder decoder;
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);

    decoder.decode(frame);

    QCOMPARE(rollingSpy.count(), 1);
    const QList<QVariant> rollingArguments = rollingSpy.takeFirst();
    const QPair<float, float> rolling =
        rollingArguments.at(0).value<QPair<float, float>>();
    QCOMPARE(rolling.first, expectedDx);
    QCOMPARE(rolling.second, expectedDy);
}

void TestDecoder::decode_truncated_frames_data()
{
    QTest::addColumn<int>("totalSize");

    for (int totalSize = 0; totalSize < 27; ++totalSize)
    {
        const QByteArray rowName = QString("truncated-%1").arg(totalSize).toUtf8();
        QTest::newRow(rowName.constData()) << totalSize;
    }
}

void TestDecoder::decode_truncated_frames()
{
    QFETCH(int, totalSize);

    QByteArray frame = buildDynamicFrame(0x60, 0x00, 0x00, 0x00, 0, 0, totalSize);

    const QString exe = QCoreApplication::applicationFilePath();
    QByteArray hex = frame.toHex();
    QProcess proc;
    proc.start(exe, QStringList() << "--subproc-decode" << QString::fromUtf8(hex));
    bool finished = proc.waitForFinished(2000);
    QVERIFY2(finished, "subprocess did not finish in time");

    // NormalExit and code 0 means child ran decode() without aborting.
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QCOMPARE(proc.exitCode(), 0);
}

// ----- QEK -----
void TestDecoder::decode_qek_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedCount");
    QTest::addColumn<QString>("expectedQek");

    QTest::newRow("QEK example 0x10") << buildDynamicFrame(0x20, 0xFF, 0x10, 0x01, 0, 0) << 1 << QString("QEK_20");
    QTest::newRow("QEK example 0x11") << buildDynamicFrame(0x20, 0xFF, 0x11, 0x01, 0, 0) << 1 << QString("QEK_21");
    QTest::newRow("QEK example 0x20") << buildDynamicFrame(0x20, 0xFF, 0x20, 0x01, 0, 0) << 1 << QString("QEK_40");
    QTest::newRow("QEK none") << buildDynamicFrame(0x20, 0xFF, 0x00, 0x01, 0, 0) << 0 << QString();
    QTest::newRow("QEK unknown") << buildDynamicFrame(0x20, 0xFF, 0x30, 0x01, 0, 0) << 0 << QString();
}

void TestDecoder::decode_qek()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedCount);
    QFETCH(QString, expectedQek);

    ConcDecoder decoder;
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    decoder.decode(frame);

    QCOMPARE(qekSpy.count(), expectedCount);
    if (expectedCount == 1)
    {
        const QList<QVariant> args = qekSpy.takeFirst();
        QCOMPARE(args.at(0).toString(), expectedQek);
    }
}

void TestDecoder::decode_handwheel_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<float>("expectedPhi");
    QTest::addColumn<float>("expectedRho");

    QByteArray positive = buildDynamicFrame(0x20, 0x00, 0x10, 0x01, 0, 0);
    setHandWheelBytes(positive, 0x7F, 0x80);
    QTest::newRow("Handwheel positive-negative") << positive << 127.0f << -128.0f;

    QByteArray negativePositive = buildDynamicFrame(0x20, 0x00, 0x10, 0x01, 0, 0);
    setHandWheelBytes(negativePositive, 0xFE, 0x03);
    QTest::newRow("Handwheel negative-positive") << negativePositive << -2.0f << 3.0f;

    QByteArray zeroes = buildDynamicFrame(0x20, 0x00, 0x10, 0x01, 0, 0);
    setHandWheelBytes(zeroes, 0x00, 0x00);
    QTest::newRow("Handwheel zeroes") << zeroes << 0.0f << 0.0f;

    QByteArray extremes = buildDynamicFrame(0x20, 0x00, 0x10, 0x01, 0, 0);
    setHandWheelBytes(extremes, 0x80, 0x7F);
    QTest::newRow("Handwheel extremes") << extremes << -128.0f << 127.0f;
}

void TestDecoder::decode_handwheel()
{
    QFETCH(QByteArray, frame);
    QFETCH(float, expectedPhi);
    QFETCH(float, expectedRho);

    ConcDecoder decoder;
    QSignalSpy handwheelSpy(&decoder, &ConcDecoder::newHandWheel);

    decoder.decode(frame);

    QCOMPARE(handwheelSpy.count(), 1);
    const QList<QVariant> args = handwheelSpy.takeFirst();
    const QPair<float, float> handwheel = args.at(0).value<QPair<float, float>>();
    QCOMPARE(handwheel.first, expectedPhi);
    QCOMPARE(handwheel.second, expectedRho);
}

// ----- Overlay -----
void TestDecoder::decode_overlay_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<QString>("expectedOverlay");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("Overlay SPC") << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 0, 0) << QString("SPC") << 1;
    QTest::newRow("Overlay LINCO") << buildDynamicFrame(0x00, 0xFF, 0x10, 0x02, 0, 0) << QString("LINCO") << 1;
    QTest::newRow("Overlay invalid zero") << buildDynamicFrame(0x00, 0xFF, 0x10, 0x00, 0, 0) << QString() << 0;
}

void TestDecoder::decode_overlay()
{
    QFETCH(QByteArray, frame);
    QFETCH(QString, expectedOverlay);
    QFETCH(int, expectedCount);

    ConcDecoder decoder;
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);

    decoder.decode(frame);

    QCOMPARE(overlaySpy.count(), expectedCount);
    if (expectedCount == 1)
    {
        const QList<QVariant> args = overlaySpy.takeFirst();
        QCOMPARE(args.at(0).toString(), expectedOverlay);
    }
}

void TestDecoder::decode_multi_signal()
{
    // Construimos una trama que contiene range, rolling y qek/overlay válidos
    QByteArray frame = buildDynamicFrame(0x60, 0xFF, 0x11, 0x02, 5, -3);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    QCOMPARE(rollingSpy.count(), 1);
    QCOMPARE(qekSpy.count(), 1);
    QCOMPARE(overlaySpy.count(), 1);
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("LINCO"));
}

void TestDecoder::decode_malformed_header_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");
    QTest::addColumn<int>("expectedQekCount");
    QTest::addColumn<int>("expectedOverlayCount");
    QTest::addColumn<QString>("expectedQek");
    QTest::addColumn<QString>("expectedOverlay");

    // Trama con header corrupto, pero campos útiles intactos.
    // El contrato actual del decoder es seguir procesando la trama.
    QByteArray badHeader = buildDynamicFrame(0xAA, 0xBB, 0x10, 0x01, 0, 0);

    // Trama completamente en cero: sigue siendo procesada por el decoder actual.
    QByteArray allZero = QByteArray(27, 0);

    QTest::newRow("Header corrupt")
        << badHeader
        << 64
        << 1
        << 1
        << QString("QEK_20")
        << QString("SPC");

    QTest::newRow("All zeroes header")
        << allZero
        << 2
        << 0
        << 0
        << QString()
        << QString();
}

void TestDecoder::decode_malformed_header()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedRange);
    QFETCH(int, expectedQekCount);
    QFETCH(int, expectedOverlayCount);
    QFETCH(QString, expectedQek);
    QFETCH(QString, expectedOverlay);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), expectedRange);

    QCOMPARE(qekSpy.count(), expectedQekCount);
    if (expectedQekCount == 1)
    {
        QCOMPARE(qekSpy.takeFirst().at(0).toString(), expectedQek);
    }

    QCOMPARE(overlaySpy.count(), expectedOverlayCount);
    if (expectedOverlayCount == 1)
    {
        QCOMPARE(overlaySpy.takeFirst().at(0).toString(), expectedOverlay);
    }

    QCOMPARE(rollingSpy.count(), 1);
}

void TestDecoder::decode_word2_flags_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<QString>("expectedSignal");

    QTest::newRow("cuOrOffCentLeft") << buildDynamicFrame(0x00, 0x80, 0x00, 0x00, 0, 0) << QString("cuOrOffCentLeft");
    QTest::newRow("cuOrCentLeft") << buildDynamicFrame(0x00, 0x40, 0x00, 0x00, 0, 0) << QString("cuOrCentLeft");
    QTest::newRow("offCentLeft") << buildDynamicFrame(0x00, 0x20, 0x00, 0x00, 0, 0) << QString("offCentLeft");
    QTest::newRow("centLeft") << buildDynamicFrame(0x00, 0x10, 0x00, 0x00, 0, 0) << QString("centLeft");
    QTest::newRow("resetObmLeft") << buildDynamicFrame(0x00, 0x08, 0x00, 0x00, 0, 0) << QString("resetObmLeft");
    QTest::newRow("dataReqLeft") << buildDynamicFrame(0x00, 0x04, 0x00, 0x00, 0, 0) << QString("dataReqLeft");
    QTest::newRow("trueMotion") << buildDynamicFrame(0x00, 0x02, 0x00, 0x00, 0, 0) << QString("trueMotion");
    QTest::newRow("ownCursTrue") << buildDynamicFrame(0x00, 0x01, 0x00, 0x00, 0, 0) << QString("ownCursTrue");
}

void TestDecoder::decode_word2_flags()
{
    QFETCH(QByteArray, frame);
    QFETCH(QString, expectedSignal);

    ConcDecoder decoder;
    QSignalSpy cuOrOffSpy(&decoder, &ConcDecoder::cuOrOffCentLeft);
    QSignalSpy cuOrCentSpy(&decoder, &ConcDecoder::cuOrCentLeft);
    QSignalSpy offCentSpy(&decoder, &ConcDecoder::offCentLeft);
    QSignalSpy centSpy(&decoder, &ConcDecoder::centLeft);
    QSignalSpy resetObmSpy(&decoder, &ConcDecoder::resetObmLeft);
    QSignalSpy dataReqSpy(&decoder, &ConcDecoder::dataReqLeft);
    QSignalSpy trueMotionSpy(&decoder, &ConcDecoder::trueMotion);
    QSignalSpy ownCursSpy(&decoder, &ConcDecoder::ownCurs);

    decoder.decode(frame);

    if (expectedSignal == QString("cuOrOffCentLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 1);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("cuOrCentLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 1);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("offCentLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 1);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("centLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 1);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("resetObmLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 1);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("dataReqLeft"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 1);
        QCOMPARE(trueMotionSpy.count(), 0);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else if (expectedSignal == QString("trueMotion"))
    {
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 1);
        QCOMPARE(ownCursSpy.count(), 0);
    }
    else
    {
        QCOMPARE(ownCursSpy.count(), 1);
        const QList<QVariant> args = ownCursSpy.takeFirst();
        QCOMPARE(args.at(0).toBool(), true);
        QCOMPARE(cuOrOffSpy.count(), 0);
        QCOMPARE(cuOrCentSpy.count(), 0);
        QCOMPARE(offCentSpy.count(), 0);
        QCOMPARE(centSpy.count(), 0);
        QCOMPARE(resetObmSpy.count(), 0);
        QCOMPARE(dataReqSpy.count(), 0);
        QCOMPARE(trueMotionSpy.count(), 0);
    }
}

void TestDecoder::decode_word2_owncurs_transition()
{
    ConcDecoder decoder;
    QSignalSpy ownCursSpy(&decoder, &ConcDecoder::ownCurs);

    QByteArray activate = buildDynamicFrame(0x00, 0x01, 0x00, 0x00, 0, 0);
    QByteArray deactivate = buildDynamicFrame(0x00, 0x00, 0x00, 0x00, 0, 0);

    decoder.decode(activate);
    decoder.decode(deactivate);

    QCOMPARE(ownCursSpy.count(), 2);
    QCOMPARE(ownCursSpy.takeFirst().at(0).toBool(), true);
    QCOMPARE(ownCursSpy.takeFirst().at(0).toBool(), false);
}

void TestDecoder::decode_reserved_corrupt()
{
    QByteArray frame = buildDynamicFrame(0x20, 0x00, 0x10, 0x03, 2, -2);
    setHandWheelBytes(frame, 0x00, 0x00);

    // Introducimos basura en offsets que el decoder actual no interpreta (20..26)
    for (int i = 20; i <= 26; ++i)
        frame[i] = static_cast<char>(0xFF);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);
    QSignalSpy handwheelSpy(&decoder, &ConcDecoder::newHandWheel);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), 4);
    QCOMPARE(qekSpy.count(), 1);
    QCOMPARE(qekSpy.takeFirst().at(0).toString(), QString("QEK_20"));
    QCOMPARE(overlaySpy.count(), 1);
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("ASW"));
    QCOMPARE(handwheelSpy.count(), 1);
    const QPair<float, float> handwheel = handwheelSpy.takeFirst().at(0).value<QPair<float, float>>();
    QCOMPARE(handwheel.first, 0.0f);
    QCOMPARE(handwheel.second, 0.0f);
    QCOMPARE(rollingSpy.count(), 1);
    const QPair<float, float> rolling = rollingSpy.takeFirst().at(0).value<QPair<float, float>>();
    QCOMPARE(rolling.first, 2.0f);
    QCOMPARE(rolling.second, -2.0f);
}

void TestDecoder::decode_range_limits_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");

    QTest::newRow("Range 2") << buildDynamicFrame(0x00, 0x00, 0x00, 0x00, 0, 0) << 2;
    QTest::newRow("Range 4") << buildDynamicFrame(0x20, 0x00, 0x00, 0x00, 0, 0) << 4;
    QTest::newRow("Range 8") << buildDynamicFrame(0x40, 0x00, 0x00, 0x00, 0, 0) << 8;
    QTest::newRow("Range 16") << buildDynamicFrame(0x60, 0x00, 0x00, 0x00, 0, 0) << 16;
    QTest::newRow("Range 32") << buildDynamicFrame(0x80, 0x00, 0x00, 0x00, 0, 0) << 32;
    QTest::newRow("Range 64") << buildDynamicFrame(0xA0, 0x00, 0x00, 0x00, 0, 0) << 64;
    QTest::newRow("Range 128") << buildDynamicFrame(0xC0, 0x00, 0x00, 0x00, 0, 0) << 128;
    QTest::newRow("Range 256") << buildDynamicFrame(0xE0, 0x00, 0x00, 0x00, 0, 0) << 256;
}

void TestDecoder::decode_range_limits()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedRange);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    const QList<QVariant> args = rangeSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), expectedRange);
}

void TestDecoder::decode_burst_sequence()
{
    const int N = 50;
    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);

    for (int i = 0; i < N; ++i)
    {
        QByteArray f = buildDynamicFrame(0x60, 0xFF, 0x11, 0x02, 1, -1);
        decoder.decode(f);
    }

    // Contrato actual: el rango solo se emite cuando cambia el valor decodificado.
    // Repetir la misma trama no debe generar 50 emisiones idénticas.
    QCOMPARE(rangeSpy.count(), 1);
}

void TestDecoder::decode_repeated_identical()
{
    ConcDecoder decoder;
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    // Usamos un valor que sí existe en QEK_DECODE para que el decoder emita.
    QByteArray frame = buildDynamicFrame(0x00, 0xFF, 0x1B, 0x01, 0, 0);
    decoder.decode(frame);
    decoder.decode(frame);
    decoder.decode(frame);

    // QEK emite en cada trama válida aunque el valor no cambie.
    QCOMPARE(qekSpy.count(), 3);
}

void TestDecoder::decode_mixed_valid_and_corrupt_sequence()
{
    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);
    QSignalSpy handwheelSpy(&decoder, &ConcDecoder::newHandWheel);

    QByteArray valid1 = buildDynamicFrame(0x20, 0xFF, 0x10, 0x01, 1, 2);
    QByteArray corrupt = QByteArray(10, 0);
    QByteArray valid2 = buildDynamicFrame(0x60, 0xFF, 0x11, 0x02, -1, -2);

    decoder.decode(valid1);
    decoder.decode(corrupt);
    decoder.decode(valid2);

    QCOMPARE(rangeSpy.count(), 3);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), 4);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), 2);
    QCOMPARE(rangeSpy.takeFirst().at(0).toInt(), 16);

    QCOMPARE(qekSpy.count(), 2);
    QCOMPARE(qekSpy.takeFirst().at(0).toString(), QString("QEK_20"));
    QCOMPARE(qekSpy.takeFirst().at(0).toString(), QString("QEK_21"));

    QCOMPARE(overlaySpy.count(), 2);
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("SPC"));
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("LINCO"));

    QCOMPARE(rollingSpy.count(), 3);
    QPair<float, float> rolling1 = rollingSpy.takeFirst().at(0).value<QPair<float, float>>();
    QPair<float, float> rolling2 = rollingSpy.takeFirst().at(0).value<QPair<float, float>>();
    QPair<float, float> rolling3 = rollingSpy.takeFirst().at(0).value<QPair<float, float>>();
    QCOMPARE(rolling1.first, 1.0f);
    QCOMPARE(rolling1.second, 2.0f);
    QCOMPARE(rolling2.first, 0.0f);
    QCOMPARE(rolling2.second, 0.0f);
    QCOMPARE(rolling3.first, -1.0f);
    QCOMPARE(rolling3.second, -2.0f);

    QCOMPARE(handwheelSpy.count(), 3);
}

void TestDecoder::decode_endianness()
{
    // dx=5, dy=10 -> salida esperada (5,10)
    QByteArray normal = buildDynamicFrame(0x00, 0xFF, 0x00, 0x00, 5, 10);

    // swapped -> dx should be 10, dy 5 (verificamos que se leyó por offset, no por palabra multibyte)
    QByteArray swapped = normal;
    swapped[18] = normal[19];
    swapped[19] = normal[18];

    ConcDecoder decoderA;
    QSignalSpy sA(&decoderA, &ConcDecoder::newRollingBall);
    decoderA.decode(normal);
    QCOMPARE(sA.count(), 1);
    QPair<float, float> rA = sA.takeFirst().at(0).value<QPair<float, float>>();
    QCOMPARE(rA.first, 5.0f);
    QCOMPARE(rA.second, 10.0f);

    ConcDecoder decoderB;
    QSignalSpy sB(&decoderB, &ConcDecoder::newRollingBall);
    decoderB.decode(swapped);
    QCOMPARE(sB.count(), 1);
    QPair<float, float> rB = sB.takeFirst().at(0).value<QPair<float, float>>();
    QCOMPARE(rB.first, 10.0f);
    QCOMPARE(rB.second, 5.0f);
}

void TestDecoder::decode_signal_order()
{
    QByteArray frame = buildDynamicFrame(0x60, 0x00, 0x11, 0x02, 7, -7);
    setHandWheelBytes(frame, 0x01, 0x02);

    ConcDecoder decoder;
    QVector<QString> seq;

    QObject::connect(&decoder, &ConcDecoder::newRange, [&seq](int)
                     { seq.append("range"); });
    QObject::connect(&decoder, &ConcDecoder::newQEK, [&seq](const QString &)
                     { seq.append("qek"); });
    QObject::connect(&decoder, &ConcDecoder::newOverlay, [&seq](const QString &)
                     { seq.append("overlay"); });
    QObject::connect(&decoder, &ConcDecoder::newHandWheel, [&seq](const QPair<float, float> &)
                     { seq.append("handwheel"); });
    QObject::connect(&decoder, &ConcDecoder::newRollingBall, [&seq](const QPair<float, float> &)
                     { seq.append("rolling"); });

    decoder.decode(frame);

    QVERIFY(seq.size() >= 5);
    QCOMPARE(seq.at(0), QString("range"));
    QCOMPARE(seq.at(1), QString("qek"));
    QCOMPARE(seq.at(2), QString("overlay"));
    QCOMPARE(seq.at(3), QString("handwheel"));
    QCOMPARE(seq.at(4), QString("rolling"));
}

void TestDecoder::decode_overlay_change()
{
    ConcDecoder decoder;
    QSignalSpy overlaySpy(&decoder, &ConcDecoder::newOverlay);

    QByteArray a = buildDynamicFrame(0x00, 0xFF, 0x00, 0x05, 0, 0);
    QByteArray b = buildDynamicFrame(0x00, 0xFF, 0x00, 0x07, 0, 0);

    decoder.decode(a);
    decoder.decode(b);

    QCOMPARE(overlaySpy.count(), 2);
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("HECO"));
    QCOMPARE(overlaySpy.takeFirst().at(0).toString(), QString("AAW"));
}

// ----- Fuzzing controlado: no debe colapsar el decoder -----
void TestDecoder::decode_fuzzing_random()
{
    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);

    QRandomGenerator rng(0xC0FFEE);

    for (int i = 0; i < 100; ++i)
    {
        QByteArray f(27, 0);
        for (int b = 0; b < 27; ++b)
            f[b] = static_cast<char>(rng.bounded(256));
        decoder.decode(f);
    }

    QCOMPARE(rollingSpy.count(), 100);
    QVERIFY(rangeSpy.count() >= 1);
}

int main(int argc, char **argv)
{
    // Special helper mode: run ConcDecoder::decode() in a subprocess to
    // protect the test runner from ASSERT/abort inside the decoder.
    if (argc >= 3 && QString(argv[1]) == "--subproc-decode")
    {
        QCoreApplication app(argc, argv);
        QByteArray hex = QString::fromLocal8Bit(argv[2]).toUtf8();
        QByteArray frame = QByteArray::fromHex(hex);
        ConcDecoder decoder;
        // Run decode once; if it ASSERTs the child will crash and parent
        // will observe a non-zero exit code / crash status.
        decoder.decode(frame);
        return 0;
    }

    TestDecoder tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_decoder.moc"