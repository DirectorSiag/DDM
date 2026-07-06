#pragma once
#include "iTransport.h"
#include <QLocalSocket>
#include <QByteArray>

class LocalIpcClient : public ITransport {
    Q_OBJECT
public:
    // name: nombre del socket/pipe (p.ej. "siag_ddm")
    explicit LocalIpcClient(QString name, QObject* parent=nullptr);

    void start() override;
    void stop() override;
    bool send(const QByteArray& data) override;
    bool isConnected() const override;

private:
    void onReadyRead();
    static QByteArray pack(const QByteArray& payload); // [len(uint32 LE)] + payload

    QString name_;
    QLocalSocket* sock_ = nullptr;
    QByteArray buf_;

private slots:
    void attemptConnect();
};
