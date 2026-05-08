#pragma once

#include <QString>
#include <QJsonArray>

class CommandContext;

/**
 * @brief Solicitud para crear un nuevo cursor (línea de azimut)
 * 
 * Contiene todos los parámetros requeridos para construir una CursorEntity.
 * Validación básica ocurre en CursorService::createCursor().
 */
struct CursorCreateRequest {
    double azimut = 0.0;  ///< Ángulo en grados (0-359.9)
    double length = 0.0;  ///< Longitud de la línea (0-256)
    double x = 0.0;       ///< Coordenada X de origen
    double y = 0.0;       ///< Coordenada Y de origen
    int type = 0;         ///< Tipo de cursor (0-7)
};

/**
 * @brief Resultado de operación sobre cursores
 * 
 * Contiene estado de éxito/falló, códigos de error, y en caso de éxito,
 * datos completos de la entidad creada para evitar búsquedas posteriores.
 */
struct CursorOperationResult {
    bool success = false;      ///< True si operación completó sin errores
    QString errorCode;         ///< Código de error estandarizado (ej: "INVALID_TYPE")
    QString message;           ///< Descripción legible del error o estado
    int cursorId = -1;         ///< ID asignado al cursor (-1 si falla)
    QString lineId;            ///< ID línea en formato "LINE_XXX"
    // Entity data (populated on success)
    double x = 0.0;           ///< Coordenada X confirmada del cursor
    double y = 0.0;           ///< Coordenada Y confirmada del cursor
    double angle = 0.0;       ///< Ángulo confirmado (normalizado)
    double length = 0.0;      ///< Longitud confirmada
    int type = 0;             ///< Tipo confirmado
    bool active = false;      ///< Estado de actividad
};

/**
 * @brief Servicio para CRUD de cursores (líneas de azimut)
 * 
 * Encapsula operaciones de creación, eliminación y serialización de cursores,
 * delegando mutaciones al CommandContext a través de métodos públicos específicos.
 */
class CursorService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit CursorService(CommandContext* context);

    /**
     * @brief Crea un nuevo cursor en el contexto
     * @param request Estructura con parámetros del nuevo cursor
     * @return CursorOperationResult con estado, ID, y datos confirmados de la entidad
     * @note Valida: tipo (0-7), longitud (0-256), coordenadas dentro de rango
     * @note En caso de éxito, result deberá contener todos los datos de la entidad
     */
    CursorOperationResult createCursor(const CursorCreateRequest& request);

    /**
     * @brief Elimina un cursor por ID
     * @param cursorId ID del cursor a eliminar
     * @return CursorOperationResult indicando éxito o error (cursor no encontrado)
     */
    CursorOperationResult deleteCursorById(int cursorId);

    /**
     * @brief Serializa todos los cursores actuales a JSON
     * @return QJsonArray con objetos JSON representando cada cursor
     * @note Cada objeto contiene: id, type, angle, length, x, y, active
     */
    QJsonArray serializeCursors() const;

    /**
     * @brief Convierte un ID de cursor a su representación de línea
     * @param cursorId ID numérico del cursor
     * @return String en formato "LINE_<ID>" (ej: "LINE_5")
     * @note Función estática, no requiere instancia
     */
    static QString lineIdFromCursorId(int cursorId);

    /**
     * @brief Convierte un ID de línea a su ID de cursor numérico
     * @param lineId String en formato "LINE_<ID>"
     * @param ok Puntero a bool que recibe estado de conversión (opcional)
     * @return ID numérico del cursor, o -1 si conversión falla
     * @note Función estática, no requiere instancia
     */
    static int cursorIdFromLineId(const QString& lineId, bool* ok = nullptr);

private:
    CommandContext* m_context;
};
