#include "concDecoder.h"
#include <QDebug>
#include <QBitArray>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

#define WORD_SIZE 24
#define WORD_COUNT 9

ConcDecoder::ConcDecoder() {}

void ConcDecoder::decode(const QByteArray &message)
{
    currentBit = 0;
    readJson();
    inComingMessage = new QBitArray();
    *inComingMessage = byteArrayToBitArray(message);

    decodeWord1();
    decodeWord2();
    //decodeWord3();
    decodeWord4();
    decodeWord5();
    decodeWord6();
    decodeWord7();
}


void ConcDecoder::decodeWord1()
{
    currentBit = WORD_SIZE * 0;;

    QString rangeBits;
    for (int i = 0; i <= 2; ++i) {
        rangeBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    QJsonObject rangeJson = jsonFile["RANGE_SCALE_DECODE"].toObject();
    if (rangeJson.contains(rangeBits)) {
        QString decodedRange = rangeJson[rangeBits].toString();
        const auto parts = decodedRange.split('_');
        if (parts.size() >= 3) {
            int lastRange = parts.at(1).toInt(); //juajua hackerman
            if(lastRange != range){
                this->range = lastRange;
                //qDebug() << range;
                emit newRange(range);
            }
        }
    }

    // --- DISPLAY SELECTION (bits 3-11) ---
    QVector<bool> decodedDisplaySelection;
    QJsonObject displayJson = jsonFile["DISPLAY_SELECTION_DECODE"].toObject();
    for (int i = 0; i < 8; i++) {
        if (inComingMessage->testBit(currentBit)) {
            QString key = QString::number(currentBit);
            if (displayJson.contains(key)) {
                QString decoded = displayJson[key].toString();
            }
            decodedDisplaySelection.append(true); //aca tengo que ver que pongo
        } else {
            decodedDisplaySelection.append(false);
        }
        currentBit++;
    }

    this->displaySelection = decodedDisplaySelection;

    // --- THREAT ASSESSMENT (bits 12-16) ---
    QVector<bool> decodedThreatAssessment;
    QJsonObject threatJson = jsonFile["THREAT_ASSESMENT_DECODE"].toObject();
    for (int i = 0; i < 5; ++i) {
        if (inComingMessage->testBit(currentBit)) {
            QString key = QString::number(currentBit);
            if (threatJson.contains(key)) {
                QString decoded = threatJson[key].toString();
            }
            decodedThreatAssessment.append(true);
        } else {
            decodedThreatAssessment.append(false);
        }
        currentBit++;
    }

    this->threatAssessment = decodedThreatAssessment;
}


// ====================== MENSAJE 2 ======================
void ConcDecoder::decodeWord2()
{
    currentBit = WORD_SIZE * 1;

    QVector<bool> decoded(16, false);
    const QJsonObject centerJson = jsonFile["CENTER_MASTER_DECODE"].toObject();

    // Guardamos el estado anterior del bit 7 para detectar flanco descendente
    bool prevBit7 = prevCenterLeft[7];

    // ---------- LADO IZQUIERDO: bits 0..7 ----------
    for (int i = 0; i < 8; ++i) {
        const bool now = inComingMessage->testBit(currentBit);
        decoded[i] = now;

        // (Opcional) log/lookup del JSON
        if (now) {
            const QString key = QString::number(currentBit);
            if (centerJson.contains(key)) {
                const QString txt = centerJson[key].toString();
                // qDebug() << "[CENTER IZQ]" << i << "->" << txt;
            }
        }

        // Flanco ascendente para los bits 0..6 (señales del enum)
        if (i <= 6 && !prevCenterLeft[i] && now) {
            switch (i) {
            case 0: emit cuOrOffCentLeft(); break;
            case 1: emit cuOrCentLeft();    break;
            case 2: emit offCentLeft();     break;
            case 3: emit centLeft();        break;
            case 4: emit resetObmLeft();    break;
            case 5: emit dataReqLeft();     break;
            case 6: emit trueMotion();      break;
            }
        }

        // Bit 7: detectar ambos flancos → true al subir, false al bajar
        if (i == 7) {
            if (!prevBit7 && now) {
                emit ownCurs(true);   // flanco ascendente: activar
            } else if (prevBit7 && !now) {
                emit ownCurs(false);  // flanco descendente: desactivar
            }
        }

        // Actualizar memoria y avanzar
        prevCenterLeft[i] = now;
        ++currentBit;
    }

    // Si querés **forzar** el estado cada ciclo (no solo en cambios), descomentá:
    // if (!decoded[7]) emit ownCurs(false);
}


// ====================== MENSAJE 4 ======================
void ConcDecoder::decodeWord4()
{
    int word4 = WORD_SIZE * 3;
    currentBit = word4;

    QString qekMasterBits;
    for (int i = 0; i < 8; i++) {
        qekMasterBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    //QEK Master
    QJsonObject qekJson = jsonFile["QEK_DECODE"].toObject();
    if (qekJson.contains(qekMasterBits)) {
        QString decodedQEK = qekJson[qekMasterBits].toString();
        if(!decodedQEK.contains("QEK_NONE")){
            quickEntryKeyboardMaster.clear();
            for (QChar bit : qekMasterBits) {
                quickEntryKeyboardMaster.append(bit == '1');
            }
            qDebug() << "[Decodificación] QEK Master:" << qekMasterBits << "→" << decodedQEK;
            emit newQEK(decodedQEK);
        }
    } else {
        //qDebug() << "[Decodificación] QEK Master desconocido:" << qekMasterBits;
    }

    // QEK Slave
    QString qekSlaveBits;
    for (int i = 0; i < 8; i++) {
        //qDebug()<<"El bit actual es: "<< currentBit;
        //qDebug()<< "Tiene valor: "<< inComingMessage->testBit(currentBit);
        qekSlaveBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    if (qekJson.contains(qekSlaveBits)) {
        QString decodedQEK = qekJson[qekSlaveBits].toString();
        quickEntryKeyboardSlave.clear();
        //qDebug() << "[Decodificación] QEK Slave:" << qekSlaveBits << "→" << decodedQEK;
    } else {
        //qDebug() << "[Decodificación] QEK Slave desconocido:" << qekSlaveBits;
    }
}


void ConcDecoder::decodeWord5()
{
    int word5 = WORD_SIZE * 4;
    currentBit = word5;

    // --- ICM Izquierdo (bits 0–2 de la palabra 5) ---
    QString icmMasterBits;
    for (int i = 0; i <= 2; i++) {
        icmMasterBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    QJsonObject icmJson = jsonFile["ICM_DECODE"].toObject();

    if (icmJson.contains(icmMasterBits)) {
        QString decodedICM = icmJson[icmMasterBits].toString();
        this->icmMaster.clear();
        for (QChar bit : icmMasterBits) {
            this->icmMaster.append(bit == '1');
        }
        //qDebug() << "[Decodificación] ICM Izquierdo:" << icmMasterBits << "→" << decodedICM;
    } else {
        //qWarning() << "[Decodificación] ICM Izquierdo desconocido:" << icmMasterBits;
    }

    currentBit++; // hay un 0 en el bit 3

    // --- Overlay Izquierdo (bits 4–7 palabra 5) ---
    QString overlayMasterBits;
    for (int i = 0; i <= 3; i++) {
        overlayMasterBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    QJsonObject overlayJson = jsonFile["OVERLAY_DECODE"].toObject();
    if (overlayJson.contains(overlayMasterBits)) {
        QString decodedOverlay = overlayJson[overlayMasterBits].toString();
        if(decodedOverlay.compare(this->overlayMaster) != 0){
            this->overlayMaster = decodedOverlay;
            emit newOverlay(overlayMaster);
        }


    } else {
        qWarning() << "[Decodificación] Overlay Izquierdo desconocido:" << overlayMasterBits;
    }

    // --- ICM Derecho (bits 8–10 de la palabra 5) ---
    QString icmSlaveBits;
    for (int i = 0; i <= 2; i++) {
        icmSlaveBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    if (icmJson.contains(icmSlaveBits)) {
        QString decodedICM = icmJson[icmSlaveBits].toString();
        this->icmSlave = decodedICM;
        //qDebug() << "[Decodificación] ICM Derecho:" << icmSlaveBits << "→" << decodedICM;
    } else {
        //qWarning() << "[Decodificación] ICM Derecho desconocido:" << icmSlaveBits;
    }

    currentBit++; // salto bit 11

    // --- Overlay Derecho (bits 12–15 palabra 5) ---
    QString overlaySlaveBits;
    for (int i = 0; i <= 3; i++) {
        overlaySlaveBits.append(inComingMessage->testBit(currentBit) ? '1' : '0');
        currentBit++;
    }

    if (overlayJson.contains(overlaySlaveBits)) {
        QString decodedOverlay = overlayJson[overlaySlaveBits].toString();
        this->overlaySlave = decodedOverlay;
        //qDebug() << "[Decodificación] Overlay Derecho:" << overlaySlaveBits << "→" << decodedOverlay;
    } else {
        //qWarning() << "[Decodificación] Overlay Derecho desconocido:" << overlaySlaveBits;
    }
}

void ConcDecoder::decodeWord6()
{
    // La palabra 6 empieza en el bit 24 * 5 = 120
    int word6 = WORD_SIZE * 5;  // Si las palabras van de 0–8, mantener 5; si van de 1–9, cambiar a 6
    currentBit = word6;

    // --- Handwheel ΔΦ (bits 0–7 palabra 6) ---
    float phiValue = 0.0f;
    for (int i = 0; i < 8; ++i) {
        if (currentBit >= inComingMessage->size()) {
            //qWarning() << "[Decodificación] Error: acceso fuera de rango en ΔΦ (bit" << currentBit << ")";
            break;
        }
        bool bit = inComingMessage->testBit(currentBit);
        phiValue = (phiValue < 1) | (bit ? 1 : 0);
        currentBit++;
    }

    // Conversión de complemento a dos (signed 8-bit)
    if (phiValue && 0x80) {
        phiValue -= 256;
    }

    // --- Handwheel Δρ (bits 8–15 palabra 6) ---
    int rhoValue = 0.0f;
    for (int i = 0; i < 8; ++i) {
        if (currentBit >= inComingMessage->size()) {
            //qWarning() << "[Decodificación] Error: acceso fuera de rango en Δρ (bit" << currentBit << ")";
            break;
        }
        bool bit = inComingMessage->testBit(currentBit);
        rhoValue = (rhoValue << 1) | (bit ? 1 : 0);
        currentBit++;
    }

    if (rhoValue & 0x80) {
        rhoValue -= 256;
    }

    // Guardamos en los estados internos
    this->handWheelPhiQueue = phiValue;
    this->handWheelRhoQueue = rhoValue;

    QPair<float,float> deltasHandwhell;
    deltasHandwhell.first = phiValue;
    deltasHandwhell.second = rhoValue;

    //qDebug() << "[Decodificación] Handwheel ΔΦ:" << phiValue
    //         << " Δρ:" << rhoValue;

    emit newHandWheel(deltasHandwhell);
}


static inline quint32 readBitsMSB(const QBitArray* bits, int start, int n) {
    quint32 v = 0;
    for (int i = 0; i < n; ++i) {
        if (start + i >= bits->size()) break;
        v = (v << 1) | (bits->testBit(start + i) ? 1u : 0u);
    }
    return v;
}

static inline qint32 signExtend(quint32 x, unsigned bits) {
    const quint32 sign = 1u << (bits - 1);
    x &= (sign << 1) - 1;              // enmascara a 'bits' bits
    return static_cast<qint32>((x ^ sign) - sign);
}

void ConcDecoder::decodeWord7()
{
    // Palabra 7 → desplazamiento base 24 * 6 = 144
    const int word7 = WORD_SIZE * 6;   // WORD_SIZE=24
    int bit = word7;

    // Lee 8 bits MSB-first y hace sign-extend (int8)
    const quint32 rawDx = readBitsMSB(inComingMessage, bit, 8); bit += 8;
    const quint32 rawDy = readBitsMSB(inComingMessage, bit, 8); bit += 8;

    const qint32 dx = signExtend(rawDx, 8);   // p.ej. 0xEA (234) -> -22
    const qint32 dy = signExtend(rawDy, 8);

    // Si querés guardar los bytes crudos (tal cual vinieron) en la cola:
    RollingSteps step;
    step.first  = QByteArray(1, static_cast<char>(static_cast<quint8>(rawDx)));
    step.second = QByteArray(1, static_cast<char>(static_cast<quint8>(rawDy)));

    // Emite los valores ya con signo correcto
    emit newRollingBall(QPair<int,int>(dx, dy));
}

void ConcDecoder::decodeWord8()
{
    // Palabra 8 → desplazamiento base 24 * 7 = 168
    int word8 = WORD_SIZE * 7;
    currentBit = word8;

    // --- Rolling Ball ΔX (bits 0–7 palabra 8) ---
    int deltaX = 0;
    for (int i = 0; i < 8; ++i) {
        if (currentBit >= inComingMessage->size()) {
            //qWarning() << "[Decodificación] Error: acceso fuera de rango en ΔX (bit" << currentBit << ")";
            break;
        }
        bool bit = inComingMessage->testBit(currentBit);
        deltaX = (deltaX << 1) | (bit ? 1 : 0);
        currentBit++;
    }

    // --- Rolling Ball ΔY (bits 8–15 palabra 8) ---
    int deltaY = 0;
    for (int i = 0; i < 8; ++i) {
        if (currentBit >= inComingMessage->size()) {
            //qWarning() << "[Decodificación] Error: acceso fuera de rango en ΔY (bit" << currentBit << ")";
            break;
        }
        bool bit = inComingMessage->testBit(currentBit);
        deltaY = (deltaY << 1) | (bit ? 1 : 0);
        currentBit++;
    }

    // Guardamos en la cola de Rolling Ball Slave
    RollingSteps step;
    step.first = QByteArray(1, static_cast<char>(deltaX));
    step.second = QByteArray(1, static_cast<char>(deltaY));
    // this->rollingBallSlave.enqueue(step);

    //qDebug() << "[Decodificación] Rolling Ball Slave ΔX:" << deltaX
    //         << " ΔY:" << deltaY;
}



 // ====================== UTILIDADES ======================
 QBitArray ConcDecoder::byteArrayToBitArray(const QByteArray &byteArray)
 {
     QBitArray bits(byteArray.size() * 8);
     for (int i = 0; i < byteArray.size(); ++i) {
         for (int bit = 0; bit < 8; ++bit) {
             bool value = (byteArray[i] >> (7 - bit)) & 1;
             bits.setBit(i * 8 + bit, value);
         }
     }
     return bits;
 }

 QByteArray ConcDecoder::bitArrayToByteArray(const QBitArray &bitArray)
 {
     QByteArray byteArray((bitArray.size() + 7) / 8, 0);
     for (int i = 0; i < bitArray.size(); ++i) {
         byteArray[i / 8] |= (bitArray.testBit(i) ? 1 : 0) << (7 - (i % 8));
     }
     return byteArray;
 }

 void ConcDecoder::readJson()
 {
     QString jsonFilePath = ":/jsons/decodificado.json";
     QFile file(jsonFilePath);
     if (file.open(QIODevice::ReadOnly)) {
         QByteArray bytes = file.readAll();
         file.close();
         QJsonParseError jsonError;
         QJsonDocument document = QJsonDocument::fromJson(bytes, &jsonError);
         if (jsonError.error == QJsonParseError::NoError && document.isObject()) {
             jsonFile = document.object();
         } else {
             //qWarning() << "Error al leer JSON:" << jsonError.errorString();
         }
     } else {
         //qWarning() << "No se pudo abrir el archivo JSON:" << jsonFilePath;
     }
 }
