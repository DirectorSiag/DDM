#include <QSignalSpy>
#include <QTest>
#include <QPair>

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

    void decode_corrupt_frame_is_ignored_data();
    void decode_corrupt_frame_is_ignored();
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

    // Escenarios Nominales de Escala (Bits 24, 23, 22 del manual)
    QTest::newRow("Escala Mínima - 2 DM") << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 0, 0) << 2;
    QTest::newRow("Escala Intermedia - 4 DM") << buildDynamicFrame(0x01, 0xFF, 0x10, 0x01, 0, 0) << 4;
    QTest::newRow("Escala Táctica - 16 DM") << buildDynamicFrame(0x05, 0xFF, 0x10, 0x01, 0, 0) << 16;
    QTest::newRow("Escala Máxima - 256 DM") << buildDynamicFrame(0x07, 0xFF, 0x10, 0x01, 0, 0) << 256;

    // Escenarios de estrés: Ruido en la derecha e infiltración de comandos
    QTest::newRow("Escala 16 DM con datos basura en canal derecho/esclavo")
        << buildDynamicFrame(0x05, 0x00, 0x00, 0x00, -5, 12) << 16;

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

void TestDecoder::decode_corrupt_frame_is_ignored_data()
{
    QTest::addColumn<QByteArray>("frame");

    // Casos de falla total: tramas truncadas o vacías por la red UDP
    QTest::newRow("Datagrama UDP severamente truncado (10 bytes)")
        << buildDynamicFrame(0x00, 0xFF, 0x10, 0x01, 0, 0, 10);

    QTest::newRow("Trama huérfana de red completamente vacía")
        << QByteArray();
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

void TestDecoder::decode_corrupt_frame_is_ignored()
{
    QFETCH(QByteArray, frame);

    ConcDecoder decoder;
    QSignalSpy rangeSpy(&decoder, &ConcDecoder::newRange);
    QSignalSpy rollingSpy(&decoder, &ConcDecoder::newRollingBall);

    decoder.decode(frame);

    // Condición de seguridad: El contador de señales debe dar estrictamente CERO
    // El decoder debió bloquear la salida para proteger el backend local del DDM
    QCOMPARE(rangeSpy.count(), 0);
    QCOMPARE(rollingSpy.count(), 0);
}

QTEST_APPLESS_MAIN(TestDecoder)

#include "tst_decoder.moc"