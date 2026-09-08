#pragma once
#include "iTransport.h"

class clientSocket; // tu clase existente (DDM UDP)

class UdpClientAdapter : public ITransport {
    Q_OBJECT
public:
    // Toma propiedad de clientSocket (o cámbialo a raw* si preferís administrarlo afuera)
    explicit UdpClientAdapter(clientSocket* udp, QObject* parent=nullptr);
    ~UdpClientAdapter() override;

    void start() override;   // noop para UDP si no lo necesitás
    void stop() override;    // idem
    bool send(const QByteArray& data) override;

private:
    clientSocket* udp_;
};
