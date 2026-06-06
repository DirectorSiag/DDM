#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTest>

#include "commandContext.h"
#include "handlers/linecommandhandler.h"
#include "network/iTransport.h"

class FakeTransport : public ITransport
{
public:
    using ITransport::ITransport;

    bool send(const QByteArray& data) override
    {
        sentMessages.append(data);
        return true;
    }

    QList<QByteArray> sentMessages;
};

class tst_linecommandhandler : public QObject
{
    Q_OBJECT

public:
    tst_linecommandhandler();
    ~tst_linecommandhandler() override;

private slots:
    void createLine_con_argumentos_validos_crea_cursor_y_respuesta_success_data();
    void createLine_con_argumentos_validos_crea_cursor_y_respuesta_success();
    void createLine_con_argumentos_invalidos_devuelve_error_data();
    void createLine_con_argumentos_invalidos_devuelve_error();
    void deleteLine_con_id_existente_elimina_cursor_y_respuesta_success();
    void deleteLine_con_argumentos_invalidos_devuelve_error_data();
    void deleteLine_con_argumentos_invalidos_devuelve_error();
};

namespace {

QJsonObject buildCreateArgs(double azimut, double length)
{
    QJsonObject args;
    args["azimut"] = azimut;
    args["length"] = length;
    return args;
}

QJsonObject buildCreateArgs(double azimut, double length, int type, double x, double y)
{
    QJsonObject args = buildCreateArgs(azimut, length);
    args["type"] = type;
    args["x"] = x;
    args["y"] = y;
    return args;
}

QJsonObject buildDeleteArgs(const QString& id)
{
    QJsonObject args;
    args["id"] = id;
    return args;
}

}

tst_linecommandhandler::tst_linecommandhandler() {}

tst_linecommandhandler::~tst_linecommandhandler() = default;

void tst_linecommandhandler::createLine_con_argumentos_validos_crea_cursor_y_respuesta_success_data()
{
    QTest::addColumn<QJsonObject>("args");
    QTest::addColumn<int>("expectedType");

    QTest::newRow("con campos opcionales")
        << buildCreateArgs(90.0, 12.5, 3, 10.0, 20.0)
        << 3;

    QTest::newRow("solo campos obligatorios")
        << buildCreateArgs(45.0, 20.0)
        << 0;
}

void tst_linecommandhandler::createLine_con_argumentos_validos_crea_cursor_y_respuesta_success()
{
    QFETCH(QJsonObject, args);
    QFETCH(int, expectedType);

    CommandContext context;
    FakeTransport transport;
    LineCommandHandler handler(&context, &transport);

    const QByteArray rawResponse = handler.createLine(args);

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(rawResponse, &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);

    const QJsonObject response = responseDoc.object();
    const QJsonObject responseArgs = response.value("args").toObject();
    const QJsonArray lines = responseArgs.value("lines").toArray();
    const QJsonObject createdLine = lines.at(0).toObject();

    QCOMPARE(response.value("status").toString(), QString("success"));
    QCOMPARE(response.value("command").toString(), QString("create_line"));
    QCOMPARE(responseArgs.value("created_id").toString(), QString("LINE_2"));
    QCOMPARE(lines.count(), 1);
    QCOMPARE(createdLine.value("id").toString(), QString("LINE_2"));
    QCOMPARE(createdLine.value("type").toInt(), expectedType);
    QCOMPARE(createdLine.value("active").toBool(), true);

    QCOMPARE(static_cast<int>(context.getCursors().size()), 1);
    QCOMPARE(context.getCursors().front().getCursorId(), 2);
    QCOMPARE(context.nextCursorId, 3);

    QCOMPARE(transport.sentMessages.count(), 0);
}

void tst_linecommandhandler::createLine_con_argumentos_invalidos_devuelve_error_data()
{
    QTest::addColumn<QJsonObject>("args");
    QTest::addColumn<QString>("expectedCode");

    {
        QJsonObject args;
        args["length"] = 10.0;
        QTest::newRow("sin azimut") << args << QString("INVALID_AZIMUT");
    }

    {
        QJsonObject args;
        args["azimut"] = 90.0;
        QTest::newRow("sin length") << args << QString("INVALID_LENGTH");
    }

    QTest::newRow("azimut menor al minimo")
        << buildCreateArgs(-1.0, 10.0)
        << QString("INVALID_AZIMUT");

    QTest::newRow("azimut mayor al maximo")
        << buildCreateArgs(360.0, 10.0)
        << QString("INVALID_AZIMUT");

    QTest::newRow("length menor al minimo")
        << buildCreateArgs(90.0, 0.0)
        << QString("INVALID_LENGTH");

    QTest::newRow("length mayor al maximo")
        << buildCreateArgs(90.0, 300.0)
        << QString("INVALID_LENGTH");

    QTest::newRow("type menor al minimo")
        << buildCreateArgs(90.0, 10.0, -1, 0.0, 0.0)
        << QString("INVALID_TYPE");

    QTest::newRow("type mayor al maximo")
        << buildCreateArgs(90.0, 10.0, 8, 0.0, 0.0)
        << QString("INVALID_TYPE");
}

void tst_linecommandhandler::createLine_con_argumentos_invalidos_devuelve_error()
{
    QFETCH(QJsonObject, args);
    QFETCH(QString, expectedCode);

    CommandContext context;
    FakeTransport transport;
    LineCommandHandler handler(&context, &transport);

    const QByteArray rawResponse = handler.createLine(args);

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(rawResponse, &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);

    const QJsonObject response = responseDoc.object();
    const QJsonObject responseArgs = response.value("args").toObject();

    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("command").toString(), QString("create_line"));
    QCOMPARE(responseArgs.value("code").toString(), expectedCode);

    QCOMPARE(static_cast<int>(context.getCursors().size()), 0);
    QCOMPARE(context.nextCursorId, 2);
    QCOMPARE(transport.sentMessages.count(), 0);
}

void tst_linecommandhandler::deleteLine_con_id_existente_elimina_cursor_y_respuesta_success()
{
    CommandContext context;
    FakeTransport transport;
    LineCommandHandler handler(&context, &transport);

    QJsonObject createArgs;
    createArgs["azimut"] = 45.0;
    createArgs["length"] = 20.0;
    handler.createLine(createArgs);

    QJsonObject deleteArgs;
    deleteArgs["id"] = "LINE_2";

    const QByteArray rawResponse = handler.deleteLine(deleteArgs);

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(rawResponse, &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);

    const QJsonObject response = responseDoc.object();
    const QJsonObject responseArgs = response.value("args").toObject();

    QCOMPARE(response.value("status").toString(), QString("success"));
    QCOMPARE(response.value("command").toString(), QString("delete_line"));
    QCOMPARE(responseArgs.value("deleted_id").toString(), QString("LINE_2"));
    QCOMPARE(responseArgs.value("lines").toArray().count(), 0);

    QCOMPARE(static_cast<int>(context.getCursors().size()), 0);
    QCOMPARE(transport.sentMessages.count(), 0);
}

void tst_linecommandhandler::deleteLine_con_argumentos_invalidos_devuelve_error_data()
{
    QTest::addColumn<QJsonObject>("args");
    QTest::addColumn<QString>("expectedCode");

    QTest::newRow("sin id")
        << QJsonObject()
        << QString("MISSING_REQUIRED_FIELDS");

    QTest::newRow("id vacio")
        << buildDeleteArgs("")
        << QString("MISSING_REQUIRED_FIELDS");

    QTest::newRow("formato sin prefijo")
        << buildDeleteArgs("2")
        << QString("INVALID_ID_FORMAT");

    QTest::newRow("numero invalido")
        << buildDeleteArgs("LINE_ABC")
        << QString("INVALID_ID_FORMAT");

    QTest::newRow("linea inexistente")
        << buildDeleteArgs("LINE_99")
        << QString("LINE_NOT_FOUND");
}

void tst_linecommandhandler::deleteLine_con_argumentos_invalidos_devuelve_error()
{
    QFETCH(QJsonObject, args);
    QFETCH(QString, expectedCode);

    CommandContext context;
    FakeTransport transport;
    LineCommandHandler handler(&context, &transport);

    handler.createLine(buildCreateArgs(45.0, 20.0));

    const QByteArray rawResponse = handler.deleteLine(args);

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(rawResponse, &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);

    const QJsonObject response = responseDoc.object();
    const QJsonObject responseArgs = response.value("args").toObject();

    QCOMPARE(response.value("status").toString(), QString("error"));
    QCOMPARE(response.value("command").toString(), QString("delete_line"));
    QCOMPARE(responseArgs.value("code").toString(), expectedCode);

    QCOMPARE(static_cast<int>(context.getCursors().size()), 1);
    QCOMPARE(context.getCursors().front().getCursorId(), 2);
    QCOMPARE(transport.sentMessages.count(), 0);
}

QTEST_APPLESS_MAIN(tst_linecommandhandler)

#include "tst_linecommandhandler.moc"
