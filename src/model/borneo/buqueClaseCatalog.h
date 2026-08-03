#pragma once

#include <QMap>
#include <QPair>
#include <QList>
#include <QString>

// Catalogo de clases de buque
class BuqueClaseCatalog
{
public:
    // Devuelve true y completa outEslora si la clase existe en el catalogo.
    static bool resolveEslora(const QString& clase, double& outEslora);

    // Enumera el catalogo completo (clase, eslora) para exponerlo a la UI.
    static QList<QPair<QString, double>> allClases();
};