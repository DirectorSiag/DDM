#pragma once
#include <QString>
#include <QStringList>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "model/derrotas/pasadaCalculator.h" // DerrotasLogPoint

class DerrotasLogManager {
public:
    explicit DerrotasLogManager(const QString& logDirectory);

    // abre un nuevo archivo log para el TN indicado.
    bool startLog(int trackId, const QDateTime& startTime, int segmentMinutes);

    // Escribe un punto en el archivo actualmente abierto.
    bool writePoint(const DerrotasLogPoint& point);

    // si paso el intervalo de segmentMinutes desde el
    // inicio del segmento actual, cierra el archivo y abre uno nuevo.
    // Devuelve true si genero un nuevo segmento.
    bool rotateIfNeeded(const QDateTime& now);

    //cierra el archivo actual (si hay uno abierto).
    void closeLog();

    //tope estricto de 24hs desde el inicio de la grabacion.
    bool exceededMaxDuration(const QDateTime& now) const;

    // lee un log o una lista de logs concatenados
    // (varios segmentos de una misma derrota prolongada) y devuelve los
    // puntos en orden cronologico.
    static bool loadLog(const QString& filePath, QList<DerrotasLogPoint>& out_points);
    static bool loadLogs(const QStringList& filePaths, QList<DerrotasLogPoint>& out_points);

    QString currentFileName() const { return m_currentFileName; }
    bool isRecording() const { return m_file.isOpen(); }

private:
    QString buildFileName(int trackId, const QDateTime& timestamp) const;

    QString     m_logDirectory;
    QFile       m_file;
    QTextStream m_stream;
    int         m_trackId = 0;
    QDateTime   m_recordingStartTime;
    QDateTime   m_currentSegmentStartTime;
    int         m_segmentMinutes = 0;
    QString     m_currentFileName;
};