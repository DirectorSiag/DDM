#pragma once

#include <QString>
#include <vector>
#include <deque>

class CommandContext;
class Track;

/**
 * @brief Servicio de consultas complejas sobre el estado táctico
 * 
 * Proporciona funciones de búsqueda, filtrado y conteo sin modificar el contexto.
 * Útil para reportes, estadísticas y validaciones en tiempo real.
 */
class QueryService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit QueryService(CommandContext* context);

    /**
     * @brief Busca todos los tracks de un tipo específico
     * @param type Tipo de track (ej: "F", "E", "U" según enums)
     * @return std::deque<Track> con tracks coincidentes (copia, no referencia)
     * @note Retorna vacío si no hay coincidencias
     */
    std::deque<Track> findTracksByType(const QString& type) const;

    /**
     * @brief Busca todos los tracks de una identidad específica
     * @param identity Identidad del track (ej: "F", "U", "P", "S", "J")
     * @return std::deque<Track> con tracks coincidentes (copia, no referencia)
     * @note Retorna vacío si no hay coincidencias
     */
    std::deque<Track> findTracksByIdentity(const QString& identity) const;

    /**
     * @brief Obtiene copia de todos los tracks
     * @return std::deque<Track> con todos los tracks actuales
     */
    std::deque<Track> findAllTracks() const;
    
    /**
     * @brief Cuenta el número de tracks actuales
     * @return Cantidad de tracks en el contexto
     */
    int countTracks() const;

    /**
     * @brief Cuenta el número de cursores actuales
     * @return Cantidad de cursores en el contexto
     */
    int countCursors() const;

private:
    CommandContext* m_context;
};
