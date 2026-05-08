// overlayhandler.cpp
#include "overlayHandler.h"
#include "aaw.h"
#include "asw.h"
#include "ew.h"
#include "heco.h"
#include "linco.h"
#include "ops.h"
#include "spc.h"
#include "apc.h"
#include <QDebug>

OverlayHandler::OverlayHandler(QObject* parent) : QObject(parent) {}

void OverlayHandler::onNewOverlay(const QString& overlayName) {
    if (overlayName == currentOverlay && myQEK) return;
    currentOverlay = overlayName;
    myQEK = instanceNewQEK(overlayName);
    myQEK->setContext(ctx);
    myQEK->setOBMHandler(obmHandler);
    if (!myQEK) {
        qWarning() << "[OverlayHandler] Overlay desconocido:" << overlayName;
    } else {
        qDebug() << "[OverlayHandler] Overlay activo =" << overlayName;
    }
}

void OverlayHandler::onNewQEK(const QString& qekStr) {
    if (!myQEK) {
        qWarning() << "[OverlayHandler] Ignoro" << qekStr << "porque no hay overlay activo.";
        return;
    }
    Qek qek;
    if (!qekFromString(qekStr, qek)) {
        qWarning() << "[OverlayHandler] QEK inválido:" << qekStr;
        return;
    }
    executeQEK(qek);
}

void OverlayHandler::onNewCursor()
{

}

void OverlayHandler::ownCursOn()
{

}

std::unique_ptr<QEK> OverlayHandler::instanceNewQEK(const QString& overlayName) {
    if (overlayName.compare("SPC", Qt::CaseInsensitive) == 0) return std::make_unique<SPC>();
    if (overlayName.compare("LINCO", Qt::CaseInsensitive) == 0) return std::make_unique<LINCO>();
    if (overlayName.compare("HECO", Qt::CaseInsensitive) == 0) return std::make_unique<HECO>();
    if (overlayName.compare("APC", Qt::CaseInsensitive) == 0) return std::make_unique<APC>();
    if (overlayName.compare("ASW", Qt::CaseInsensitive) == 0) return std::make_unique<ASW>();
    if (overlayName.compare("OPS", Qt::CaseInsensitive) == 0) return std::make_unique<OPS>();
    if (overlayName.compare("AAW", Qt::CaseInsensitive) == 0) return std::make_unique<AAW>();
    if (overlayName.compare("EW", Qt::CaseInsensitive) == 0) return std::make_unique<EW>();
    return nullptr;
}

void OverlayHandler::executeQEK(Qek which) {
    // Despacho por número (rápido y explícito)
    switch (static_cast<int>(which)) {
    case 20: myQEK->execute20(); break;
    case 21: myQEK->execute21(); break;
    case 22: myQEK->execute22(); break;
    case 23: myQEK->execute23(); break;
    case 24: myQEK->execute24(); break;
    case 25: myQEK->execute25(); break;
    case 26: myQEK->execute26(); break;
    case 27: myQEK->execute27(); break;

    case 30: myQEK->execute30(); break;
    case 31: myQEK->execute31(); break;
    case 32: myQEK->execute32(); break;
    case 33: myQEK->execute33(); break;
    case 34: myQEK->execute34(); break;
    case 35: myQEK->execute35(); break;
    case 36: myQEK->execute36(); break;
    case 37: myQEK->execute37(); break;

    case 40: myQEK->execute40(); break;
    case 41: myQEK->execute41(); break;
    case 42: myQEK->execute42(); break;
    case 43: myQEK->execute43(); break;
    case 44: myQEK->execute44(); break;
    case 45: myQEK->execute45(); break;
    case 46: myQEK->execute46(); break;
    case 47: myQEK->execute47(); break;

    case 50: myQEK->execute50(); break;
    case 51: myQEK->execute51(); break;
    case 52: myQEK->execute52(); break;
    case 53: myQEK->execute53(); break;
    case 54: myQEK->execute54(); break;
    case 55: myQEK->execute55(); break;
    case 56: myQEK->execute56(); break;
    case 57: myQEK->execute57(); break;

    default:
        qWarning() << "[OverlayHandler] QEK no soportado:" << static_cast<int>(which);
    }
}
