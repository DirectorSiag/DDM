#include "ITransport.h"
#include "TransportFactory.h"
#include "json/jsoncommandhandler.h"
#include "lpdEncoder.h"
#include "messagerouter.h"
#include "obmHandler.h"
#include "dclConcController.h"
#include "concDecoder.h"
#include "overlayHandler.h"
#include <QCoreApplication>
#include <QTextStream>
#include <QThread>
#include <QObject>
#ifdef Q_OS_WIN
    #include <windows.h>
#endif
#include <stdinreader.h>

#include "CommandDispatcher.h"
#include "commandRegistry.h"
#include "commandParser.h"
#include "addCommand.h"
#include "deleteCommand.h"
#include "centerCommand.h"
#include "listCommand.h"
#include "commandContext.h"
#include "QTimer"
#include "configuration.h"
#include "addCursor.h"
#include "listcursorscommand.h"
#include "deletecursorscommand.h"
#include "sitrepcommand.h"

static void enableAnsiColorsOnWindows() {
    DWORD mode = 0;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        mode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
        SetConsoleMode(hOut, mode);
    }
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    if (hErr != INVALID_HANDLE_VALUE && GetConsoleMode(hErr, &mode)) {
        mode |= 0x0004;
        SetConsoleMode(hErr, mode);
    }
}

int main(int argc, char* argv[]) {

#ifdef Q_OS_WIN
    enableAnsiColorsOnWindows();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);


    auto* ctx = new CommandContext();
    auto* registry = new CommandRegistry();
    auto* parser = new CommandParser();

    // registrar comandos
    registry->registerCommand(QSharedPointer<ICommand>(new AddCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new DeleteCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new CenterCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new ListCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new addCursor()));
    registry->registerCommand(QSharedPointer<ICommand>(new ListCursorsCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new DeleteCursorsCommand()));
    registry->registerCommand(QSharedPointer<ICommand>(new SitrepCommand()));

    CommandDispatcher dispatcher(registry, parser, *ctx);


    QThread ioThread;
    StdinReader reader;
    reader.moveToThread(&ioThread);

    QObject::connect(&ioThread, &QThread::started, &reader, &StdinReader::readLoop);
    QObject::connect(&reader, &StdinReader::lineRead, &dispatcher, &CommandDispatcher::onLine);
    QObject::connect(&dispatcher, &CommandDispatcher::quitRequested, &app, &QCoreApplication::quit);
    QObject::connect(&reader, &StdinReader::finished, &ioThread, &QThread::quit);

    QTextStream out(stdout);
    ctx->out << ANSI_YELLOW << "Consola lista (help | exit)" << ANSI_RESET << "\n";
    ctx->out.flush();

    encoderLPD *encoder = new encoderLPD();
    auto* decoder = new ConcDecoder();

    bool useLocalIpc = Configuration::instance().useLocalIpc;

    TransportOpts opts;
    if (useLocalIpc) {
        opts.localName = "siag_ddm";  // debe coincidir con el nombre que use el servidor (juego)
    }

    // Mantené vivo el unique_ptr (no uses release), y usá get() para el crudo
    std::unique_ptr<ITransport> transportGuard = useLocalIpc
        ? makeTransport(TransportKind::LocalIpc, opts, &app)
        : makeTransport(TransportKind::Udp,       TransportOpts{}, &app);

    ITransport* transport = transportGuard.get();
    transport->start();

    QTimer timer;
    QTimer updatePositionTimer;
    QObject::connect(&updatePositionTimer, &QTimer::timeout, [ctx, &updatePositionTimer](){

        double deltaTime = updatePositionTimer.interval() / 1000.0;
        ctx->updateTracks(deltaTime);
    });

    QObject::connect(&timer, &QTimer::timeout, &timer, [ctx, encoder, transport]() {
        transport->send(encoder->buildFullMessage(*ctx));
    });


    auto* obmHandler = new OBMHandler();
    auto* ownCurs = new OwnCurs(ctx,obmHandler);

    // 1. Crear los controladores
    auto* dclConcController = new DclConcController(transport, decoder, &app);
    auto* jsonHandler = new JsonCommandHandler(ctx, transport, &app);

    // 2. Crear el Router y pasarle los controladores
    auto* router = new MessageRouter(dclConcController, jsonHandler, &app);

    // 3. Conectar el transporte ÚNICAMENTE al router
    QObject::connect(transport, &ITransport::messageReceived,
                     router, &MessageRouter::onMessageReceived);

    auto* overlayHandler = new OverlayHandler();
    overlayHandler->setContext(ctx);
    overlayHandler->setOBMHandler(obmHandler);

    //conectar señales del decoder con ownCurse
    QObject::connect(decoder, &ConcDecoder::newHandWheel, ownCurs, &OwnCurs::updateHandwheel);
    QObject::connect(decoder, &ConcDecoder::cuOrOffCentLeft, ownCurs, &OwnCurs::cuOrOffCent);
    QObject::connect(decoder, &ConcDecoder::cuOrCentLeft, ownCurs, &OwnCurs::cuOrCent);
    QObject::connect(decoder, &ConcDecoder::ownCurs, ownCurs, &OwnCurs::ownCursActive);

    // Conecta señales que emite el decoder
    QObject::connect(decoder, &ConcDecoder::newOverlay, overlayHandler, &OverlayHandler::onNewOverlay);
    QObject::connect(decoder, &ConcDecoder::newQEK, overlayHandler, &OverlayHandler::onNewQEK);




    QObject::connect(decoder, &ConcDecoder::newRange, obmHandler, &OBMHandler::updateRange);
    QObject::connect(decoder, &ConcDecoder::newRollingBall, obmHandler, &OBMHandler::updatePosition);
    encoder->setOBMHandler(obmHandler);

    QObject::connect(decoder, &ConcDecoder::offCentLeft,  [ctx,obmHandler](){
        ctx->setCenter(obmHandler->getPosition());
    });

    QObject::connect(decoder, &ConcDecoder::centLeft,  [ctx](){
        ctx->resetCenter();
    });

    QObject::connect(decoder, &ConcDecoder::resetObmLeft,  [obmHandler](){
        obmHandler->setPosition({0.0,0.0});
    });

    QObject::connect(decoder, &ConcDecoder::dataReqLeft,  [obmHandler, ctx](){
        Track* t = obmHandler->OBMAssociationProcess(ctx);
        if(t) qDebug() << t->toString();
    });

    timer.start(40);
    updatePositionTimer.start(80);

    ioThread.start();
    const int code = app.exec();
    ioThread.wait();
    return code;
}
