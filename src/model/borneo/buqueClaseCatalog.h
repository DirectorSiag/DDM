#pragma once

#include <QMap>
#include <QString>

// Catalogo de clases de buque
class BuqueClaseCatalog
{
public:
    // Devuelve true y completa outEslora si la clase existe en el catalogo.
    static bool resolveEslora(const QString& clase, double& outEslora);
};