#ifndef LPDENCODER_H
#define LPDENCODER_H

#include "commandContext.h"
#include "obmHandler.h"
#include <QObject>
#include <QByteArray>
#include <QPair>
#include <QMap>

using Type      = TrackData::Type;
using Identity  = TrackData::Identity;
using TrackMode = TrackData::TrackMode;

class encoderLPD
{
public:
    explicit encoderLPD() = default;
    QByteArray buildFullMessage(const CommandContext &ctx);
    void setOBMHandler(OBMHandler* oh){obmHandler = oh;}

private:
    void appendAB2Message(QByteArray& dst, const Track &track);
    void appendAB3Message(QByteArray& dst, const CursorEntity &cursor);
    void appendCoordinate(QByteArray& dst, double value, uint8_t idBits, bool AP=true, bool PV=false, bool LS=false);
    void appendAngle(QByteArray& dst, double value, bool e, bool v);
    void appendCursorLong(QByteArray& dst, double value, int type);
    void appendSymbolBytes(QByteArray& dst, const Track &track) const;
    void appendOBM(QByteArray& dst);

    // Helpers puros (sin cambios)
    QPair<uint8_t, uint8_t> symbolFor(const Track &track) const;
    uint8_t trackModeFor(const Track &track) const;

    // Invertir in-place
    void negateDataInPlace(QByteArray &data, int startPos);

    OBMHandler *obmHandler{nullptr};
};

constexpr uint8_t BIT_LS = 1 << 6;
constexpr uint8_t BIT_PV = 1 << 5;
constexpr uint8_t BIT_V  = 1 << 4;
constexpr uint8_t BIT_AP = 1 << 3;
constexpr uint8_t BIT_E =  1 << 6;


constexpr quint8 AB3_ID_ANGLE = 0x05;
constexpr quint8 AB3_ID_RHO   = 0x07;

constexpr quint8 AB3_ID_X     = 0x06;
constexpr quint8 AB3_ID_Y     = 0x04;

constexpr uint8_t AB2_ID_X = 0x01;
constexpr uint8_t AB2_ID_Y = 0x03;

constexpr uint8_t AB1_ID_X = 0x09;
constexpr uint8_t AB1_ID_Y = 0x0B;


constexpr uint8_t EOMM = 0x17;

constexpr char HEADER_BYTES[]       = { 0x00, 0x00 };
constexpr char DESCENTRADO_BYTES[]  = { 0x00, 0x00, 0x19, 0x00, 0x00, 0x0B };
constexpr char PADDING_BYTES[]      = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
constexpr char INVALID_OWNSHIP[]   = { 0x00, 0x00, 0x61, 0x00, 0x00, 0x23, 0x1C, 0x00, 0x17 };

#endif // LPDENCODER_H
