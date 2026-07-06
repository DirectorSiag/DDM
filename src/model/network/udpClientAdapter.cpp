#include "udpClientAdapter.h"
#include "clientSocket.h"    // tu header real

#include <QTimer>

UdpClientAdapter::UdpClientAdapter(clientSocket* udp, QObject* parent)
    : ITransport(parent),
    udp_(udp)
{
    Q_ASSERT(udp_ && "UdpClientAdapter requiere un clientSocket válido");
    connect(udp_,SIGNAL(receivedDatagram(QByteArray)),this,SIGNAL(messageReceived(QByteArray)));
}

UdpClientAdapter::~UdpClientAdapter() = default;

void UdpClientAdapter::start() {
}

void UdpClientAdapter::stop() {
}

bool UdpClientAdapter::send(const QByteArray& data) {
    if (!udp_) return false;
    udp_->sendMessage(data);
    return true;
}
