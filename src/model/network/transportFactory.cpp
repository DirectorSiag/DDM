#include "transportFactory.h"
#include "iTransport.h"
#include "udpClientAdapter.h"
#include "clientSocket.h"       // tu UDP real
#include "localipcclient.h"

#include <QUrl>
#include <QHostAddress>

static void parseUdp(const QString& uri, QHostAddress& ip, quint16& port) {
    // Soporta "udp://127.0.0.1:5000" o "127.0.0.1:5000"
    QString s = uri;
    if (s.startsWith("udp://", Qt::CaseInsensitive)) {
        QUrl u(s);
        ip   = QHostAddress(u.host());
        port = quint16(u.port());
    } else {
        const auto parts = s.split(':');
        ip   = QHostAddress(parts.value(0));
        port = quint16(parts.value(1).toUShort());
    }
}

std::unique_ptr<ITransport> makeTransport(TransportKind kind, const TransportOpts& o, QObject* parent) {
    switch (kind) {
    case TransportKind::Udp: {
        // Construí TU clientSocket como siempre; NO-owning adapter
        auto* udp = new clientSocket(parent);                 // << puntero crudo + parent
        return std::unique_ptr<ITransport>(new UdpClientAdapter(udp, parent));
    }
    case TransportKind::LocalIpc: {
        QString name = o.localName;
        if (name.startsWith("local://", Qt::CaseInsensitive))
            name = name.mid(QStringLiteral("local://").size());
        return std::unique_ptr<ITransport>(new LocalIpcClient(name, parent));
    }
    }
    return nullptr;
}
