#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "qhostaddress.h"
#include <QString>
#include <QJsonArray>

#define PORT_NOTEBOOK 8001
#define PORT_FPGA 9000
#define PORT_RADAR 8000
#define IP_FPGA "172.16.0.100"
#define IP_MASTER "172.16.0.99" // Dirección IP del maestro (local o remota)
#define IP_SLAVE "172.16.0.101"

#define PORT_MASTER 6340 // Puerto del socket UDP del maestro
#define PORT_SLAVE 6340

#define MAC_ADDRESS "34BDFA99B530"

#define DA_CONCENTRADOR 0x04
#define DA_AND1 0x02
#define DA_AND2 0x03
#define DA_LPD 0x00
#define DA_PEDIDO_CONCENTRADOR 0x01
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RESET   "\x1b[0m"

class Configuration {
public:
    // Instancia estática de la clase (Singleton)
    static Configuration& instance() {
        static Configuration instance;
        return instance;
    }

    // Configuración
    bool master;
    int meko;
    QString overlay;
    QJsonArray jsonArray;
    int midi = -1;
    bool useLocalIpc = true;

    // Métodos para cargar o establecer configuración
    void loadConfiguration();
    void setOverlay(QString);
    void setMidi(int);
    QString getOverlayKey() const;

    static const QHostAddress masterIpDhcNetwork;
    static const QHostAddress slaveIpDhcNetwork;
    static const QHostAddress masterIpRadarNetwork;
    static const QHostAddress slaveIpRadarNetwork;
    static const QHostAddress ipFpga;

    static const QHostAddress SPERRY;
    static const QHostAddress SCAN_CONVERTER;

private:
    // Constructor privado para garantizar que solo haya una instancia
    Configuration() {}

    // Eliminar el constructor de copia y operador de asignación
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
};

#endif // CONFIGURATION_H
