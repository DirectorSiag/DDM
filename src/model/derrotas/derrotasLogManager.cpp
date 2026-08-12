#include "derrotasLogManager.h"
#include <QDir>
#include <QStringConverter>

DerrotasLogManager::DerrotasLogManager(const QString& logDirectory)
    : m_logDirectory(logDirectory)
{}

QString DerrotasLogManager::buildFileName(int trackId, const QDateTime& timestamp) const
{
    // Formato basado en el ejemplo de la spec, pero con ':' reemplazado por '-': los dos puntos no son validos en nombres de
    // archivo en Windows.
    return QStringLiteral("tn%1_%2.txt")
        .arg(trackId, 4, 10, QChar('0'))
        .arg(timestamp.toString(QStringLiteral("HH-mm-ss")));
}

bool DerrotasLogManager::startLog(int trackId, const QDateTime& startTime, int segmentMinutes)
{
    if (m_file.isOpen()) {
        closeLog();
    }

    QDir dir(m_logDirectory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    m_trackId                 = trackId;
    m_recordingStartTime      = startTime;
    m_currentSegmentStartTime = startTime;
    m_segmentMinutes          = segmentMinutes;
    m_currentFileName         = buildFileName(trackId, startTime);

    m_file.setFileName(dir.filePath(m_currentFileName));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);

    // Encabezado de columnas:
    // Nombre, HORA (fecha+hora completa), Latitud,
    // Longitud, RV, VD.
    m_stream << QStringLiteral("Nombre\tFechaHora\tLatitud\tLongitud\tRV\tVD\n");
    m_stream.flush();

    return true;
}

bool DerrotasLogManager::writePoint(const DerrotasLogPoint& point)
{
    if (!m_file.isOpen()) {
        return false;
    }

    m_stream << point.trackName << '\t'
             << point.timestamp.toString(QStringLiteral("ddMMyy HH:mm:ss")) << '\t'
             << QString::number(point.latDeg, 'f', 6) << '\t'
             << QString::number(point.lonDeg, 'f', 6) << '\t'
             << QString::number(point.rvDeg, 'f', 1) << '\t'
             << QString::number(point.vdKn, 'f', 1) << '\n';

    m_stream.flush();
    return true;
}

bool DerrotasLogManager::rotateIfNeeded(const QDateTime& now)
{
    if (!m_file.isOpen() || m_segmentMinutes <= 0) {
        return false;
    }

    const qint64 elapsedSecs = m_currentSegmentStartTime.secsTo(now);
    if (elapsedSecs < static_cast<qint64>(m_segmentMinutes) * 60) {
        return false;
    }

    const int trackId = m_trackId;
    const QDateTime startTime = m_recordingStartTime;
    const int segmentMinutes = m_segmentMinutes;

    closeLog();
    // Reabrir sin perder registros entre el fin de un log y el inicio del
    // siguiente (criterio de aceptacion).
    startLog(trackId, startTime, segmentMinutes);
    m_currentSegmentStartTime = now;

    return true;
}

void DerrotasLogManager::closeLog()
{
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

bool DerrotasLogManager::exceededMaxDuration(const QDateTime& now) const
{
    //tope maximo estricto de 24 horas desde el inicio de la grabacion.
    return m_recordingStartTime.secsTo(now) >= (24 * 60 * 60);
}

bool DerrotasLogManager::loadLog(const QString& filePath, QList<DerrotasLogPoint>& out_points)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    bool firstLine = true;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (firstLine) {
            // Salteamos el encabezado de columnas.
            firstLine = false;
            continue;
        }
        if (line.trimmed().isEmpty()) continue;

        const QStringList cols = line.split('\t');
        if (cols.size() < 6) continue; // linea corrupta/incompleta

        DerrotasLogPoint p;
        p.trackName = cols[0];
        p.timestamp = QDateTime::fromString(cols[1], QStringLiteral("ddMMyy HH:mm:ss"));
        p.latDeg    = cols[2].toDouble();
        p.lonDeg    = cols[3].toDouble();
        p.rvDeg     = cols[4].toDouble();
        p.vdKn      = cols[5].toDouble();

        out_points.append(p);
    }

    return true;
}

bool DerrotasLogManager::loadLogs(const QStringList& filePaths, QList<DerrotasLogPoint>& out_points)
{
    // lectura concatenada — permite graficar una derrota prolongada sin tener que abrir los segmentos uno por uno desde la UI.
    out_points.clear();
    for (const QString& path : filePaths) {
        QList<DerrotasLogPoint> segment;
        if (!loadLog(path, segment)) {
            return false;
        }
        out_points += segment;
    }
    return true;
}