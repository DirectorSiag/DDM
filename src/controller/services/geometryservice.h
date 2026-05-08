#pragma once

#include <QPointF>
#include <QString>
#include <QJsonObject>
#include <vector>

class CommandContext;

/**
 * @brief Resultado de operación sobre figuras geométricas
 * 
 * Contiene estado de éxito/fallo y datos de la entidad creada.
 */
struct GeometryResult {
    bool success = false;      ///< True si operación completó sin errores
    QString errorCode;         ///< Código de error (ej: "DUPLICATE_ID", "INVALID_SHAPE")
    QString message;           ///< Descripción legible del resultado o error
    int id = -1;               ///< ID de la figura creada (-1 si falla)
};

/**
 * @brief Servicio para gestión de figuras geométricas (áreas, círculos, polígonos)
 * 
 * Encapsula operaciones CRUD de geometría, aplicando validaciones y manteniendo
 * consistencia del estado de entidades geométricas.
 */
class GeometryService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit GeometryService(CommandContext* context);

    /**
     * @brief Crea un área (polígono de 4 puntos) en el contexto
     * @param points Vector de QPointF con las esquinas del área (exactamente 4 puntos esperados)
     * @param areaType Tipo de área (entero identificador)
     * @param areaColor Color en formato string (ej: "#FF0000", "red")
     * @return GeometryResult con estado, ID asignado y mensaje de error si aplica
     * @note Valida: número de puntos, coordenadas dentro de rango
     */
    GeometryResult createArea(const std::vector<QPointF>& points, int areaType, const QString& areaColor);

    /**
     * @brief Elimina un área por ID
     * @param areaId ID del área a eliminar
     * @return GeometryResult indicando éxito o error (área no encontrada)
     */
    GeometryResult deleteArea(int areaId);

    /**
     * @brief Crea un círculo en el contexto
     * @param center Centro del círculo en coordenadas del radar
     * @param radius Radio del círculo (distancia)
     * @param type Tipo de círculo (entero identificador)
     * @param color Color en formato string
     * @return GeometryResult con estado, ID asignado y mensaje de error si aplica
     * @note Valida: radio > 0, coordenadas dentro de rango
     */
    GeometryResult createCircle(const QPointF& center, double radius, int type, const QString& color);

    /**
     * @brief Elimina un círculo por ID
     * @param circleId ID del círculo a eliminar
     * @return GeometryResult indicando éxito o error (círculo no encontrado)
     */
    GeometryResult deleteCircle(int circleId);

    /**
     * @brief Crea un polígono de N puntos en el contexto
     * @param points Vector de QPointF con los vértices del polígono (mínimo 3)
     * @param polyType Tipo de polígono (entero identificador)
     * @param polyColor Color en formato string
     * @return GeometryResult con estado, ID asignado y mensaje de error si aplica
     * @note Valida: mínimo 3 puntos, coordenadas dentro de rango
     */
    GeometryResult createPolygon(const std::vector<QPointF>& points, int polyType, const QString& polyColor);

    /**
     * @brief Elimina un polígono por ID
     * @param polygonId ID del polígono a eliminar
     * @return GeometryResult indicando éxito o error (polígono no encontrado)
     */
    GeometryResult deletePolygon(int polygonId);

    /**
     * @brief Serializa todas las figuras geométricas actuales
     * @return QJsonObject con listas de áreas, círculos y polígonos
     * @note Formato: { "areas": [...], "circles": [...], "polygons": [...] }
     */
    QJsonObject listShapes() const;

private:
    CommandContext* m_context;
};
