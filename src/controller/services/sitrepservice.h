#pragma once

#include <deque>
#include <QString>

class CommandContext;
class Track;

/**
 * @brief Servicio especializado para SITREP (Situation Reports)
 * 
 * Gestiona información táctico-operacional asociada a tracks, incluyendo
 * información ampliatoria, códigos privados y estado de enlace.
 */
class SitrepService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit SitrepService(CommandContext* context);

    /**
     * @brief Obtiene una copia de todos los tracks actuales
     * @return std::deque<Track> con snapshot de tracks (copia, no referencia)
     */
    std::deque<Track> snapshot() const;

    /**
     * @brief Elimina un track y sus datos SITREP asociados
     * @param id ID del track a eliminar
     * @return true si el track existía y fue eliminado, false en caso contrario
     * @note Limpia automáticamente sitrepExtra asociado
     */
    bool deleteTrackById(int id);

    /**
     * @brief Busca un track por ID
     * @param id ID del track a buscar
     * @return Puntero a Track si existe, nullptr en caso contrario
     * @note Permite modificación directa de la entidad encontrada
     */
    Track* findTrackById(int id) const;

    /**
     * @brief Asigna información ampliatoria a un track
     * @param id ID del track
     * @param text Texto de información ampliatoria (información táctica)
     * @note Almacena en sitrepExtra y en el track; es idempotente
     */
    void setSitrepInfo(int id, const QString& text);

private:
    CommandContext* m_context;
};
