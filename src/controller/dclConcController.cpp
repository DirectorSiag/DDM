#include "dclConcController.h"
#include "ITransport.h"
#include "concDecoder.h"
#include <QtEndian>
#include <QDebug>

DclConcController::DclConcController(ITransport* link,
                                     ConcDecoder* decodificator,
                                     QObject* parent)
    : QObject(parent),
    m_link(link),
    m_decoder(decodificator)
{
    Q_ASSERT(m_link);
    Q_ASSERT(m_decoder);

    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &DclConcController::askForConcentrator);
    m_timer.start();
}

void DclConcController::askForConcentrator()
{
    m_link->send(buildPedidoDclConc());
}

void DclConcController::onDatagram(const QByteArray& datagram)
{
    if (datagram.size() < 3) return;

    // w1 = B0 B1 B2 (big-endian a 24 bits)
    const uchar b0 = static_cast<uchar>(datagram[0]);
    const uchar b1 = static_cast<uchar>(datagram[1]);
    const uchar b2 = static_cast<uchar>(datagram[2]);
    const quint32 w1 = (static_cast<quint32>(b0) << 16) |
                       (static_cast<quint32>(b1) << 8)  |
                       (static_cast<quint32>(b2));

    const quint16 seq = static_cast<quint16>(w1 & 0x7FFF);

    const QByteArray ack = buildAckFromSeq(seq);
    m_link->send(ack);

    if (datagram.size() > 3) {
        QByteArray payload = datagram.mid(3);
        payload = negateData(payload);
        m_decoder->decode(payload);
    }
}

QByteArray DclConcController::buildPedidoDclConc()
{
    return QByteArray::fromHex("010000012E2F");
}

QByteArray DclConcController::buildAckFromSeq(quint16 seq)
{
    // Campo de 16 bits: MSB=1 (ACK) | 15 bits de secuencia
    quint16 ackField = static_cast<quint16>(0x8000 | (seq & 0x7FFF));

    QByteArray out;
    out.reserve(3);
    out.append(char(0x04));                        // CONC_ID
    out.append(char((ackField >> 8) & 0xFF));      // MSB del campo ACK|SEQ
    out.append(char(ackField & 0xFF));             // LSB del campo ACK|SEQ
    return out;
}

QByteArray DclConcController::negateData(const QByteArray &data) {
    QByteArray invertedData = data;
    for (char &b : invertedData)
        b = ~b;
    return invertedData;
}
