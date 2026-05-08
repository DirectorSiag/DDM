#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

class CommandContext;

class EstacionamientoService
{
public:
    explicit EstacionamientoService(CommandContext* context);

    struct CalculationResult {
        bool success = false;
        QString errorMessage;
        int trackAId = -1;
        int trackBId = -1;
        double azimuthDeg = 0.0;
        double distanceDm = 0.0;
        QString modalidad;
        double modalidadValue = 0.0;
        double rumboDeg = 0.0;
        double timeHours = 0.0;
        QString timeHms;
        double stationPosXDm = 0.0;
        double stationPosYDm = 0.0;
    };

    struct OperationResult {
        bool success = false;
        QString message;
    };

    CalculationResult calculateFromOptions(const QMap<QString, QString>& options) const;
    OperationResult executeFromCliArgs(const QStringList& args) const;

private:
    struct CliInput {
        int trackAId = 0;
        int trackBId = -1;

        double azRelativeDeg = 0.0;
        double distanceDm = 0.0;

        bool hasVd = false;
        bool hasDu = false;
        double vdKnots = 0.0;
        double duHours = 0.0;
    };

    static constexpr double kDmToNm = 0.98747;

    static bool parseOptions(const QStringList& args,
                             QMap<QString, QString>& options,
                             QString& error);
    static bool validateAllowedOptions(const QMap<QString, QString>& options,
                                       QString& error);
    static bool parseDoubleField(const QMap<QString, QString>& options,
                                 const QString& key,
                                 bool required,
                                 double& value,
                                 QString& error);
    static bool parseIntegerField(const QMap<QString, QString>& options,
                                  const QString& key,
                                  bool required,
                                  int& value,
                                  QString& error);
    static bool loadAndValidate(const QMap<QString, QString>& options,
                                CliInput& input,
                                QString& error);

    static QString formatDurationHms(double hours);
    static double knotsToDmPerHour(double knots);

    CommandContext* m_context = nullptr;
};
