#include "configuration.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "configuration.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "configuration.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

const QHostAddress Configuration::masterIpDhcNetwork("172.16.0.99");
const QHostAddress Configuration::slaveIpDhcNetwork("172.16.0.101");
const QHostAddress Configuration::masterIpRadarNetwork("192.168.7.41");
const QHostAddress Configuration::slaveIpRadarNetwork("192.168.7.42");

const QHostAddress Configuration::ipFpga("172.16.0.100");

const QHostAddress Configuration::SPERRY("192.168.7.50");
const QHostAddress Configuration::SCAN_CONVERTER("192.168.7.40");

void Configuration::loadConfiguration() {

    // Lee el archivo JSON con la información de los overlays
    QFile file(":/json/json/overlay.json");
    if (!file.open(QIODevice::ReadOnly)) {
        //qWarning() << "Error No se pudo abrir el archivo de configuración de overlay.";
    }

    // Lee el archivo JSON con la información de los botones
    QString JsonFilePath = ":/json/json/properties.json";
    QFile File(JsonFilePath);
    if (!File.open(QIODevice::ReadOnly)) {
        //qWarning() << "Error Hubo un error, no se abrió el archivo de propiedades";
    }
    QByteArray archivo = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(archivo);
    file.close();

    if (!document.isObject()) {
        //qWarning("El documento JSON no es un objeto.");
        return;
    }
    QJsonObject OverlayJson = document.object();

    if (!OverlayJson.contains("overlay") || !OverlayJson["overlay"].isArray()) {
        //qWarning("El objeto JSON no contiene un array 'overlay'.");
        return;
    }
    jsonArray = OverlayJson["overlay"].toArray();

    meko = OverlayJson["tipo"].toString().toInt();
    master = OverlayJson["master"].toBool();
}

void Configuration::setOverlay(QString o) {
    overlay = o;
}

void Configuration::setMidi(int m){
    midi = m;
}

QString Configuration::getOverlayKey() const {
    for (const QJsonValue& val : jsonArray) {
        QJsonObject obj = val.toObject();
        for (const QString& key : obj.keys()) {
            if (obj[key].toString() == overlay) {
                return key;
            }
        }
    }
    return QString(); // Retorna vacío si no se encuentra
}
