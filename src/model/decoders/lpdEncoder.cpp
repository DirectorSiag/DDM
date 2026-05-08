#include "lpdEncoder.h"
#include "commandContext.h"
#include <QByteArray>
#include <QtEndian>
#include <QtGlobal>

QByteArray encoderLPD::buildFullMessage(const CommandContext &ctx) {
    QByteArray bigBuffer;

    // Pre-reserva memoria aproximada para evitar reallocs.
    int estimatedSize = 64 + (ctx.tracks.size() * 20) + (ctx.cursors.size() * 12);
    bigBuffer.reserve(estimatedSize);

    bigBuffer.append(HEADER_BYTES, sizeof(HEADER_BYTES));

    int palabrasTracks = (ctx.tracks.size() * 6) + 2 + 9;
    bigBuffer.append(static_cast<char>(palabrasTracks & 0xFF));


    // CENTER - Escritura directa
    appendCoordinate(bigBuffer, ctx.centerX, AB1_ID_X);
    appendCoordinate(bigBuffer, ctx.centerY, AB1_ID_Y);

    bigBuffer.append(INVALID_OWNSHIP, sizeof(INVALID_OWNSHIP));
    appendOBM(bigBuffer);

    // TRACKS
    for (const auto &track : ctx.tracks) {
        appendAB2Message(bigBuffer, track);
    }

    // CURSORES
    for (const auto &cursor : ctx.cursors){
        appendAB3Message(bigBuffer, cursor);
    }

    bigBuffer.append(PADDING_BYTES, sizeof(PADDING_BYTES));
    negateDataInPlace(bigBuffer, 3);
    return bigBuffer;
}

QPair<uint8_t, uint8_t> encoderLPD::symbolFor(const Track& track) const
{
    const Type t = track.getType();
    const Identity id = track.getIdentity();

    static const QHash<Identity, QHash<Type, QPair<uint8_t,uint8_t>>> LUT = {
        {
            Identity::Pending, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1D, 0x60} },
                { Type::Air,         {0x01, 0x60} },
                { Type::Subsurface,  {0x1A, 0x60} },
            }
        },{
            Identity::PossFriend, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1D, 0x64} },
                { Type::Air,         {0x01, 0x64} },
                { Type::Subsurface,  {0x1A, 0x64} },
            }
        },{
            Identity::PossHostile, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1D, 0x7C} },
                { Type::Air,         {0x01, 0x7C} },
                { Type::Subsurface,  {0x1A, 0x7C} },
            }
        },{
            Identity::ConfFriend, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1E, 0x00} },
                { Type::Air,         {0x02, 0x00} },
                { Type::Subsurface,  {0x1A, 0x00} },
            }
        },{
            Identity::ConfHostile, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1F, 0x00} },
                { Type::Air,         {0x03, 0x00} },
                { Type::Subsurface,  {0x1B, 0x00} },
            }
        },{
            Identity::EvalUnknown, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x1D, 0x00} },
                { Type::Air,         {0x01, 0x00} },
                { Type::Subsurface,  {0x19, 0x00} },
            }
        },{
            Identity::Heli, QHash<Type, QPair<uint8_t,uint8_t>>{
                { Type::Surface,     {0x0A, 0x00} },
                { Type::Air,         {0x0A, 0x00} },
                { Type::Subsurface,  {0x0A, 0x00} },
                }
        },
    };

    // Lookup
    const auto itId = LUT.find(id);
    if (itId != LUT.end()) {
        const auto& byType = itId.value();
        const auto itT = byType.find(t);
        if (itT != byType.end()) {
            return itT.value();
        }
    }

    // Fallback: "Evaluated unknown – Surface" (era tu default {0x1D,0xE0})
    return {0x1D, 0xE0};
}

uint8_t encoderLPD::trackModeFor(const Track &track) const {

    // Definimos todas las combinaciones conocidas
    static const QMap<TrackMode,uint8_t> map = {
        {TrackMode::FC1, 0x31},
        {TrackMode::FC2, 0x32},
        {TrackMode::FC3, 0x33},
        {TrackMode::FC4, 0x34},
        {TrackMode::FC5, 0x35},
        {TrackMode::Auto, 0x2E},
        {TrackMode::TentativeAuto, 0x08},
        {TrackMode::AutomaticLost, 0x11},
        {TrackMode::RAM, 0x2B},
        {TrackMode::DR, 0x2D},
        {TrackMode::Lost, 0x1E},
        {TrackMode::Blank, 0x00},
    };

    TrackMode tm = track.getTrackMode();

    if (map.contains(tm))
        return map.value(tm);
    return 0x01;
}

//tengo que guardar todos los booleanos en el Cursor entity porque son muchos parametros
void encoderLPD::appendCoordinate(QByteArray& dst, double value, uint8_t idBits, bool AP, bool PV, bool LS) {
    int32_t scaled = static_cast<int32_t>(qRound(value * 256.0));

    constexpr int bitCount = 17;
    constexpr int maxVal = (1 << (bitCount - 1)) - 1;
    constexpr int minVal = -(1 << (bitCount - 1));
    scaled = qBound(minVal, scaled, maxVal);

    uint32_t encoded = (scaled < 0)
                           ? ((1u << bitCount) + scaled)
                           : static_cast<uint32_t>(scaled);
    encoded <<= 7;

    uint8_t controlByte = 0;

    switch (idBits & 0x0F) {
    case AB2_ID_X:
        controlByte = BIT_V;
        if(PV) controlByte |= BIT_PV;
        if(LS) controlByte |= BIT_LS;
        break;
    case AB1_ID_X:
        controlByte = BIT_V;
        break;
    case AB2_ID_Y:
        controlByte = BIT_AP;
        break;
    default:
        controlByte = 0x00;
        break;
    }
    controlByte |= (idBits & 0x0F);
    encoded |= controlByte;

    dst.append(static_cast<char>((encoded >> 16) & 0xFF));
    dst.append(static_cast<char>((encoded >> 8) & 0xFF));
    dst.append(static_cast<char>(encoded & 0xFF));
}

void encoderLPD::appendAngle(QByteArray& dst, double value, bool e, bool v)
{
    // Normalizar a [0,360)
    double a = std::fmod(value, 360.0);
    if (a < 0.0) a += 360.0;

    // LSB = 360/4096 => factor ~11.377
    uint32_t code12 = static_cast<uint32_t>(qRound(a * (4096.0 / 360.0))) & 0x0FFF;

    uint32_t control = AB3_ID_ANGLE;
    if (e) control |= BIT_E;
    if (v) control |= BIT_V;

    // [23..12] ángulo, [11..0] control
    uint32_t encoded = (code12 << 12) | (control & 0x0FFF);

    dst.append(static_cast<char>((encoded >> 16) & 0xFF));
    dst.append(static_cast<char>((encoded >> 8)  & 0xFF));
    dst.append(static_cast<char>( encoded        & 0xFF));
}

void encoderLPD::appendCursorLong(QByteArray& dst, double rho, int type)
{
    if (rho < 0.0) rho = 0.0;

    const uint32_t q = static_cast<uint32_t>(qRound(rho * 256));
    const uint32_t len16 = (q & 0xFFFFu);
    const uint32_t type3 = static_cast<uint32_t>(qBound(0, type, 7)) & 0x7u;

    // [23]=0 | [22..7]=len16 | [6..4]=type3 | [3..0]=ID(0111)
    const uint32_t word24 = (len16 << 7) | (type3 << 4) | AB3_ID_RHO;

    dst.append(static_cast<char>((word24 >> 16) & 0xFF));
    dst.append(static_cast<char>((word24 >>  8) & 0xFF));
    dst.append(static_cast<char>( word24        & 0xFF));
}

void encoderLPD::appendSymbolBytes(QByteArray& dst, const Track &track) const {
    auto [sym1, sym2] = symbolFor(track);
    dst.append(static_cast<char>(sym1));
    dst.append(static_cast<char>(sym2));

    dst.append(static_cast<char>(trackModeFor(track)));
    dst.append(static_cast<char>(0x00));
    dst.append(static_cast<char>(0x00));

    int id = track.getId();

    dst.append(static_cast<char>('0' + ((id >> 9) & 0x7)));
    dst.append(static_cast<char>('0' + ((id >> 6) & 0x7)));
    dst.append(static_cast<char>('0' + ((id >> 3) & 0x7)));
    dst.append(static_cast<char>('0' + (id & 0x7)));

    dst.append((char)0x00);
    dst.append((char)0x00);
    dst.append((char)EOMM);
}

void encoderLPD::appendAB2Message(QByteArray& dst, const Track &track) {
    appendCoordinate(dst, track.getX(), AB2_ID_X);
    appendCoordinate(dst, track.getY(), AB2_ID_Y);
    appendSymbolBytes(dst, track);
}

void encoderLPD::appendAB3Message(QByteArray& dst, const CursorEntity &cursor)
{
    appendAngle(dst, cursor.getCursorAngle(), true, cursor.isActive());
    appendCursorLong(dst, cursor.getCursorLength(), cursor.getLineType());

    auto coords = cursor.getCoordinates();
    appendCoordinate(dst, coords.first,  AB3_ID_X);
    appendCoordinate(dst, coords.second, AB3_ID_Y);
}

void encoderLPD::appendOBM(QByteArray& dst)
{
    if (!obmHandler) {
        return;
    }
    auto pos = obmHandler->getPosition();
    appendCoordinate(dst, pos.first, AB2_ID_X, true, true, true);
    appendCoordinate(dst, pos.second, AB2_ID_Y, true, true, true);

    dst.append(static_cast<char>(0x0F));
    dst.append(static_cast<char>(0x00));
    dst.append(static_cast<char>(EOMM));
}

void encoderLPD::negateDataInPlace(QByteArray &data, int startPos) {
    if (startPos >= data.size()) return;

    char* rawData = data.data();
    const int size = data.size();

    for (int i = startPos; i < size; ++i) {
        rawData[i] = ~rawData[i];
    }
}

