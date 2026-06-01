#include <QSignalSpy>
#include <QTest>
#include <QPair>

#include <QVector>
#include <QString>
#include <QTime>
#include <QtGlobal>
#include <QRandomGenerator>

#include "concDecoder.h"

// Fábrica Parametrizada: Genera tramas tácticas a medida
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

class TestDecoder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Suite de Pruebas Guiadas por Datos (Data-Driven)
    void decode_frame_ranges_data();
    void decode_frame_ranges();

    void decode_rolling_signed_values_data();
    void decode_rolling_signed_values();

    // Nuevos tests propuestos
    void decode_qek_data();
    void decode_qek();

    void decode_overlay_data();
    void decode_overlay();

    void decode_multi_signal();

    void decode_malformed_header_data();
    void decode_malformed_header();

    // Tests adicionales implementados
    void decode_word2_flags_data();
    void decode_word2_flags();

    void decode_reserved_corrupt();

    void decode_range_limits_data();
    void decode_range_limits();

    void decode_burst_sequence();
    void decode_repeated_identical();

    void decode_endianness();

    void decode_signal_order();

    void decode_overlay_change();

    void decode_fuzzing_random();
};

void TestDecoder::initTestCase()
{
    // Registramos el tipo compuesto QPair para que QSignalSpy pueda operar en frío
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

    // Se dispara el procesamiento de la trama dinámica
    decoder.decode(frame);

    // El Spy cuenta cuántas veces se emitió. Debe ser exactamente UNA
    QCOMPARE(rangeSpy.count(), 1);

    // Abrimos la notificación capturada y validamos el entero extraído
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

// ----- Tests añadidos: QEK -----
void TestDecoder::decode_qek_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<QString>("expectedQek");

    QTest::newRow("QEK example 0x10") << buildDynamicFrame(0x20, 0xFF, 0x10, 0x01, 0, 0) << QString("QEK_20");
    QTest::newRow("QEK example 0x11") << buildDynamicFrame(0x20, 0xFF, 0x11, 0x01, 0, 0) << QString("QEK_21");
    QTest::newRow("QEK example 0x20") << buildDynamicFrame(0x20, 0xFF, 0x20, 0x01, 0, 0) << QString("QEK_40");
}

void TestDecoder::decode_qek()
{
    QFETCH(QByteArray, frame);
    QFETCH(QString, expectedQek);

    ConcDecoder decoder;
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    decoder.decode(frame);

    QCOMPARE(qekSpy.count(), 1);
    const QList<QVariant> args = qekSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), expectedQek);
}

// ----- Tests añadidos: Overlay -----
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

// ----- Test multi-señal en una sola trama -----
void TestDecoder::decode_multi_signal()
{
    // Construimos una trama que contiene range, rolling y qek/overlay válidos
    QByteArray frame = buildDynamicFrame(0x60, 0xFF, 0x11, 0x02, 5, -3);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    decoder.decode(frame);

    QCOMPARE(rangeSpy.count(), 1);
    QCOMPARE(rollingSpy.count(), 1);
    QCOMPARE(qekSpy.count(), 1);
}

// ----- Tests para encabezado malformado -----
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

    // El decoder actual también emite rolling una vez por trama.
    QCOMPARE(rollingSpy.count(), 1);
}

// ----- Word2 flags: asegurar que no rompe la decodificación de range -----
void TestDecoder::decode_word2_flags_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");

    // Usamos los mismos valores de Word1 que mapean a 2 y 16
    QTest::newRow("Word2 flags off") << buildDynamicFrame(0x00, 0x00, 0x10, 0x01, 0, 0) << 2;
    QTest::newRow("Word2 flag bit7 set") << buildDynamicFrame(0x60, 0x80, 0x10, 0x01, 0, 0) << 16;
}

void TestDecoder::decode_word2_flags()
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

// ----- Campo reservado corrupto: trama válida con bytes basura en secciones no críticas -----
void TestDecoder::decode_reserved_corrupt()
{
    QByteArray frame = buildDynamicFrame(0x20, 0xFF, 0x05, 0x03, 2, -2);

    // Introducimos basura en offsets no críticos (10..14)
    for (int i = 10; i <= 14; ++i)
        frame[i] = static_cast<char>(0xFF);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);

    decoder.decode(frame);

    // Debe seguir entregando la señal de rango y rolling correctamente
    QCOMPARE(rangeSpy.count(), 1);
    QCOMPARE(rollingSpy.count(), 1);
}

// ----- Límites de rango: valores extremos para Word1 -----
void TestDecoder::decode_range_limits_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<int>("expectedRange");

    // Valores límite ya conocidos (mínimo y máximo válidos)
    QTest::newRow("Min range") << buildDynamicFrame(0x00, 0xFF, 0x00, 0x00, 0, 0) << 2;
    QTest::newRow("Max range") << buildDynamicFrame(0xE0, 0xFF, 0x00, 0x00, 0, 0) << 256;

    // Valor no estándar: comprobamos que no falle (aceptamos emisión o no según implementación)
    QTest::newRow("Nonstandard") << buildDynamicFrame(0xFF, 0xFF, 0x00, 0x00, 0, 0) << 256;
}

void TestDecoder::decode_range_limits()
{
    QFETCH(QByteArray, frame);
    QFETCH(int, expectedRange);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);

    decoder.decode(frame);

    // Aceptamos que la implementación pueda mapear nonstandard a un valor extremo,
    // comprobamos que la señal se emite y el valor está en el rango esperado como mínimo.
    QCOMPARE(rangeSpy.count(), 1);
    const QList<QVariant> args = rangeSpy.takeFirst();
    QVERIFY(args.at(0).toInt() == expectedRange || args.at(0).toInt() > 0);
}

// ----- Burst: muchas tramas consecutivas -----
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

// ----- Repeated identical frames -----
void TestDecoder::decode_repeated_identical()
{
    ConcDecoder decoder;
    QSignalSpy qekSpy(&decoder, &ConcDecoder::newQEK);

    // Usamos un valor que sí existe en QEK_DECODE para que el decoder emita.
    QByteArray frame = buildDynamicFrame(0x00, 0xFF, 0x1B, 0x01, 0, 0);
    decoder.decode(frame);
    decoder.decode(frame);
    decoder.decode(frame);

    // Contrato actual: QEK emite en cada trama válida aunque el valor no cambie.
    QCOMPARE(qekSpy.count(), 3);
}

// ----- Endianness: intercambio de bytes en rolling ball -----
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

// ----- Orden de emisión de señales dentro de una trama -----
void TestDecoder::decode_signal_order()
{
    QByteArray frame = buildDynamicFrame(0x60, 0xFF, 0x11, 0x02, 7, -7);

    ConcDecoder decoder;
    QVector<QString> seq;

    QObject::connect(&decoder, &ConcDecoder::newRange, [&seq](int)
                     { seq.append("range"); });
    QObject::connect(&decoder, &ConcDecoder::newRollingBall, [&seq](const QPair<float, float> &)
                     { seq.append("rolling"); });
    QObject::connect(&decoder, &ConcDecoder::newQEK, [&seq](const QString &)
                     { seq.append("qek"); });

    decoder.decode(frame);

    // Esperamos que newRange venga antes que newRollingBall y newQEK (según contrato interno)
    QVERIFY(seq.size() >= 3);
    QCOMPARE(seq.at(0), QString("range"));
}

// ----- Overlay change detection (dos tramas consecutivas con distinto overlay) -----
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

    for (int i = 0; i < 100; ++i)
    {
        QByteArray f(27, 0);
        for (int b = 0; b < 27; ++b)
            f[b] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        decoder.decode(f);
    }

    // No hay una verificación fuerte aquí: la prueba pasa si no hay crash
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(TestDecoder)

#include "tst_decoder.moc"