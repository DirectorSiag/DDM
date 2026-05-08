#pragma once

#include <QPair>

class CommandContext;

/**
 * @brief Servicio para gestionar el centro de presentación del radar
 * 
 * Encapsula operaciones de lectura y escritura de las coordenadas del centro (centerX, centerY)
 * en el CommandContext, validando rangos y manteniendo consistencia del estado.
 */
class CenterService {
public:
    /**
     * @brief Constructor que inyecta el contexto
     * @param context Puntero a CommandContext (no nulo)
     */
    explicit CenterService(CommandContext* context);

    /**
     * @brief Establece las coordenadas del centro de presentación
     * @param x Coordenada X del centro (rango típico: -256 a 256)
     * @param y Coordenada Y del centro (rango típico: -256 a 256)
     * @note Validación de rangos ocurre en el llamador; este método confía en entrada válida
     */
    void setCenter(double x, double y);

    /**
     * @brief Obtiene las coordenadas actuales del centro
     * @return QPair<float, float> con coordenadas (x, y) normalizadas a float
     * @note Retorna valores convertidos a float; consultar CommandContext para valores internos en double
     */
    QPair<float,float> getCenter() const;

private:
    CommandContext* m_context;
};
