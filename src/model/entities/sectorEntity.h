#ifndef SECTORENTITY_H
#define SECTORENTITY_H

#include <QString>
#include <QPointF>
#include <vector>

struct CommandContext;

enum class SectorTipo {
    ZonaExclusion,
    ZonaAlerta,
    ZonaVigilancia,
    Personalizado
};

enum class SectorColor {
    RGB1, RGB2, RGB3,
    CMYK1, CMYK2, CMYK3
};

class SectorEntity {
public:
    SectorEntity(int id,
                 double az_izq,
                 double az_der,
                 double rad_int,
                 double rad_ext,
                 SectorTipo tipo,
                 SectorColor color,
                 const QPointF& origen,
                 int id_track = 0);

    bool isValid() const;
    double apertura() const;

    // Genera los segmentos de línea que aproximan el sector y los almacena en el contexto
    void calculateAndStoreCursors(CommandContext& ctx);

    // Getters
    int         getId()      const;
    double      getAzIzq()   const;
    double      getAzDer()   const;
    double      getRadInt()  const;
    double      getRadExt()  const;
    SectorTipo  getTipo()    const;
    SectorColor getColor()   const;
    const QPointF& getOrigen() const;
    int         getIdTrack() const;
    const std::vector<int>& getCursorIds() const;

    // Setters
    void setAzIzq(double az);
    void setAzDer(double az);
    void setRadInt(double rad);
    void setRadExt(double rad);
    void setTipo(SectorTipo tipo);
    void setColor(SectorColor color);
    void setOrigen(const QPointF& origen);
    void setIdTrack(int id_track);

private:
    int         id;
    double      az_izq;
    double      az_der;
    double      rad_int;
    double      rad_ext;
    SectorTipo  tipo;
    SectorColor color;
    QPointF     origen;
    int         id_track;

    std::vector<int> cursorIds;
};

#endif // SECTORENTITY_H
