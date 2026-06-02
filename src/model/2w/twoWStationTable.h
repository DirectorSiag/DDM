#pragma once

#include <array>
#include <QtGlobal>

struct TwoWStationEntry {
    double azimuthDeg;   // Azimut verdadero desde el guía [0, 360). Nunca escala.
    double distanceNm;   // Distancia en millas náuticas con radio base = 1 MN.
};

// Índice 0 = estación 1, índice 67 = estación 68.
// Acceder siempre mediante TwoWStationTable::stationAt(int stationNumber)
// para mantener la conversión 1-based -> 0-based en un único lugar.
static constexpr std::array<TwoWStationEntry, 68> TWOW_TABLA_A = {{
    //Az      Dist      //Estacion
    {   30.0,  2.0 },   //  1
    { 330.0,  2.0 },   //  2
    {  90.0,  2.0 },   //  3
    { 270.0,  2.0 },   //  4
    { 150.0,  2.0 },   //  5
    { 210.0,  2.0 },   //  6
    {   0.0,  3.5 },   //  7
    { 330.0,  4.0 },   //  8
    {  30.0,  4.0 },   //  9
    { 300.0,  3.5 },   // 10
    {  60.0,  3.5 },   // 11
    { 270.0,  4.0 },   // 12
    {  90.0,  4.0 },   // 13
    { 240.0,  3.5 },   // 14
    { 120.0,  3.5 },   // 15
    { 210.0,  4.0 },   // 16
    { 150.0,  4.0 },   // 17
    { 180.0,  3.5 },   // 18
    {  10.0,  5.3 },   // 19
    { 350.0,  5.3 },   // 20
    {  30.0,  6.0 },   // 21
    { 330.0,  6.0 },   // 22
    {  50.0,  5.3 },   // 23
    { 310.0,  5.3 },   // 24
    {  70.0,  5.3 },   // 25
    { -1.0,  -1.0 },   // 26 (no presente)
    {  90.0,  6.0 },   // 27
    { 270.0,  6.0 },   // 28
    { 110.0,  5.3 },   // 29
    { 250.0,  5.3 },   // 30
    { 130.0,  5.3 },   // 31
    { 230.0,  5.3 },   // 32
    { 150.0,  6.0 },   // 33
    { 210.0,  6.0 },   // 34
    { 170.0,  5.3 },   // 35
    { 190.0,  5.3 },   // 36
    {   0.0,  7.0 },   // 37
    { 345.0,  7.2 },   // 38
    {  15.0,  7.2 },   // 39
    { 330.0,  8.0 },   // 40
    {  30.0,  8.0 },   // 41
    { 315.0,  7.2 },   // 42
    {  45.0,  7.2 },   // 43
    { 300.0,  7.0 },   // 44
    {  60.0,  7.0 },   // 45
    { -1.0,  -1.0 },   // 46 (no presente)
    { -1.0,  -1.0 },   // 47 (no presente)
    { -1.0,  -1.0 },   // 48 (no presente)
    { -1.0,  -1.0 },   // 49 (no presente)
    { -1.0,  -1.0 },   // 50 (no presente)
    { -1.0,  -1.0 },   // 51 (no presente)
    { 240.0,  7.0 },   // 52
    { 120.0,  7.0 },   // 53
    { 225.0,  7.2 },   // 54
    { 135.0,  7.2 },   // 55
    { 210.0,  8.0 },   // 56
    { 150.0,  8.0 },   // 57
    { 195.0,  7.2 },   // 58
    { 165.0,  7.2 },   // 59
    { 180.0,  7.0 },   // 60
    { -1.0,  -1.0 },   // 61 (no presente)
    { -1.0,  -1.0 },   // 62 (no presente)
    { -1.0,  -1.0 },   // 63 (no presente)
    { -1.0,  -1.0 },   // 64 (no presente)
    { -1.0,  -1.0 },   // 65 (no presente)
    { -1.0,  -1.0 },   // 66 (no presente)
    {  40.0,  9.0 },   // 67
    { 320.0,  9.5 },   // 68
}};

class TwoWStationTable {
public:
    // Retorna la entrada para la estación indicada (1-based).
    // Precondición: stationNumber en [1, 68].
    // Si la estación solicitada no está entre 1 y 68 devuelve una entrada inválida (0.0, 0.0)
    static const TwoWStationEntry& stationAt(int stationNumber) {
        if (stationNumber >= 1 && stationNumber <= 68) {
            return TWOW_TABLA_A[static_cast<size_t>(stationNumber - 1)];
        }
        static const TwoWStationEntry invalidStation{ -1.0,  -1.0 };
        return invalidStation;
    }

    static constexpr int stationCount() { return 68; }
};