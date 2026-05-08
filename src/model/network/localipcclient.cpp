#include "LocalIpcClient.h"
#include <QDataStream>
#include <QTimer>

LocalIpcClient::LocalIpcClient(QString name, QObject* parent)
    : ITransport(parent), name_(std::move(name)) {}

void LocalIpcClient::start() {
    if (sock_) return;
    sock_ = new QLocalSocket(this);

    connect(sock_, &QLocalSocket::connected, this, [this]() {
        qDebug() << "[LocalIpcClient] Conectado al pipe:" << name_;
        emit connected();
    });
    connect(sock_, &QLocalSocket::disconnected, this, [this]() {
        qWarning() << "[LocalIpcClient] Cliente desconectado del pipe:" << name_;
        buf_.clear();
        emit disconnected();
        // Reintenta reconectar tras desconexión
        QTimer::singleShot(2000, this, &LocalIpcClient::attemptConnect);
    });
    connect(sock_, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError socketError) {
        Q_UNUSED(socketError);
        // Solo loggear errores que no sean de "servidor no encontrado" durante reconexión
        if (socketError != QLocalSocket::ServerNotFoundError && 
            socketError != QLocalSocket::ConnectionRefusedError) {
            QString errMsg = sock_->errorString();
            qWarning() << "[LocalIpcClient] Error en el socket:" << errMsg;
            emit error(errMsg);
        }
    });
    connect(sock_, &QLocalSocket::readyRead, this, &LocalIpcClient::onReadyRead);

    // Primer intento inmediato (encola al event loop)
    QTimer::singleShot(0, this, &LocalIpcClient::attemptConnect);
}

void LocalIpcClient::attemptConnect() {
    if (!sock_) return;
    if (sock_->state() == QLocalSocket::ConnectedState) return;

    sock_->abort();                      // limpia intentos previos
    sock_->connectToServer(name_);

    // Si no conecta rápido, reintenta en 2 segundos
    if (!sock_->waitForConnected(200)) {
        QTimer::singleShot(2000, this, &LocalIpcClient::attemptConnect);
    }
}

void LocalIpcClient::stop() {
    if (!sock_) return;
    buf_.clear();
    sock_->disconnectFromServer();
    sock_->deleteLater();
    sock_ = nullptr;
}

bool LocalIpcClient::send(const QByteArray& data) {
    if (!sock_ || sock_->state() != QLocalSocket::ConnectedState) {
        return false;
    }
    
    auto frame = pack(data);
    qint64 w = sock_->write(frame);
    
    if (w != frame.size()) {
        qWarning() << "[LocalIpcClient] Error al escribir datos: escritos" << w << "de" << frame.size() << "bytes";
        return false;
    }
    
    // Verificar estado antes de flush, puede haberse desconectado durante write
    if (sock_->state() != QLocalSocket::ConnectedState) {
        return false;
    }
    
    if (!sock_->flush()) {
        // Solo loggear si el error no es por desconexión
        if (sock_->error() != QLocalSocket::UnknownSocketError) {
            qWarning() << "[LocalIpcClient] Error al hacer flush del socket:" << sock_->errorString();
        }
        return false;
    }
    
    return true;
}

void LocalIpcClient::onReadyRead() {
    buf_.append(sock_->readAll());

    // desempaquetado: [len(4 bytes LE)] + payload
    while (buf_.size() >= 4) {
        quint32 len;
        memcpy(&len, buf_.constData(), 4);
        if (buf_.size() < 4 + int(len)) break;
        QByteArray payload = buf_.mid(4, len);
        buf_.remove(0, 4 + int(len));
        emit messageReceived(payload);
    }
}

bool LocalIpcClient::isConnected() const {
    return sock_ && sock_->state() == QLocalSocket::ConnectedState;
}

QByteArray LocalIpcClient::pack(const QByteArray& payload) {
    QByteArray out;
    out.resize(4 + payload.size());
    quint32 len = payload.size();
    memcpy(out.data(), &len, 4);
    memcpy(out.data()+4, payload.constData(), payload.size());
    return out;
}
