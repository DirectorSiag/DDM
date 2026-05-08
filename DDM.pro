QT += core network
CONFIG += console c++17
TEMPLATE = app
TARGET = DDM
INCLUDEPATH += $$PWD/src
QT += core gui

RESOURCES += resources.qrc

INCLUDEPATH += \
    src/ \
    src/controller/ \
    src/controller/commands/ \
    src/controller/json/ \
    src/controller/json/validators/ \
    src/controller/handlers/ \
    src/controller/services/ \
    src/model/ \
    src/model/decoders/ \
    src/model/sitrep/ \
    src/model/network/ \
    src/model/obm/ \
    src/model/overlays/ \
    src/model/utils/ \
    src/model/enums/ \
    src/view/ \


HEADERS += \
    src/controller/commandRegistry.h \
    src/controller/commanddispatcher.h \
    src/controller/commands/addCommand.h \
    src/controller/commands/addCursor.h \
    src/controller/commands/addareacommand.h \
    src/controller/commands/addCircleCommand.h \
    src/controller/commands/addpolygonocommand.h \
    src/controller/commands/centerCommand.h \
    src/controller/commands/deleteAreaCommand.h \
    src/controller/commands/deleteCircleCommand.h \
    src/controller/commands/cpaCommand.h \
    src/controller/commands/displaymodecommand.h \
    src/controller/commands/estacionamientocommand.h \
    src/controller/commands/ownshipcommand.h \
    src/controller/commands/deleteCommand.h \
    src/controller/commands/deletecursorscommand.h \
    src/controller/commands/iCommand.h \
    src/controller/commands/listCommand.h \
    src/controller/commands/listcursorscommand.h \
    src/controller/commands/sitrepcommand.h \
    src/controller/dclConcController.h \
    src/controller/json/jsoncommandhandler.h \
    src/controller/json/jsonresponsebuilder.h \
    src/controller/json/jsonserializer.h \
    src/controller/handlers/cursorcommandhandler.h \
    src/controller/handlers/ownshipcommandhandler.h \
    src/controller/services/centerservice.h \
    src/controller/handlers/geometrycommandhandler.h \
    src/controller/services/cursorservice.h \
    src/controller/services/geometryservice.h \
    src/controller/services/sitrepservice.h \
    src/controller/services/trackpppservice.h \
    src/controller/services/trackservice.h \
    src/controller/services/cpaservice.h \
    src/controller/services/ownshipservice.h \
    src/controller/services/obmservice.h \
    src/controller/services/estacionamientoservice.h \
    src/controller/services/queryservice.h \
    src/controller/handlers/trackcommandhandler.h \
    src/controller/messagerouter.h \
    src/controller/overlayHandler.h \
    src/controller/json/validators/jsonvalidator.h \
    src/model/commandContext.h \
    src/model/cpa.h \
    src/model/pppcalculator.h \
    src/model/estacionamientocalculator.h \
    src/model/decoders/concDecoder.h \
    src/model/decoders/iDecoder.h \
    src/model/decoders/lpdEncoder.h \
    src/model/entities/cursorEntity.h \
    src/model/entities/areaEntity.h \
    src/model/entities/circleEntity.h \
    src/model/entities/polygonoentity.h \
    src/model/enums/enums.h \
    src/model/network/clientSocket.h \
    src/model/network/iTransport.h \
    src/model/network/localipcclient.h \
    src/model/network/transportFactory.h \
    src/model/network/udpClientAdapter.h \
    src/model/obm/iOBMHandler.h \
    src/model/obm/obmHandler.h \
    src/model/overlays/aaw.h \
    src/model/overlays/apc.h \
    src/model/overlays/asw.h \
    src/model/overlays/ew.h \
    src/model/overlays/heco.h \
    src/model/overlays/ops.h \
    src/model/overlays/spc.h \
    src/model/ownCursor/owncurs.h \
    src/model/overlays/linco.h \
    src/model/qek.h \
    src/model/sitrep/sitrep.h \
    src/model/utils/RadarMath.h \
    src/model/utils/configuration.h \
    src/model/entities/track.h \
    src/model/utils/consoleUtils.h \
    src/view/CommandParser.h \
    src/view/ansi.h \
    src/view/iInputParser.h \
    src/view/stdinreader.h

SOURCES += \
    src/controller/commandDispatcher.cpp \
    src/controller/commands/addCommand.cpp \
    src/controller/commands/addCursor.cpp \
    src/controller/commands/addareacommand.cpp \
    src/controller/commands/addCircleCommand.cpp \
    src/controller/commands/addpolygonocommand.cpp \
    src/controller/commands/centerCommand.cpp \
    src/controller/commands/cpaCommand.cpp \
    src/controller/commands/displaymodecommand.cpp \
    src/controller/commands/estacionamientocommand.cpp \
    src/controller/commands/delete_area_command.cpp \
    src/controller/commands/ownshipcommand.cpp \
    src/controller/services/centerservice.cpp \
    src/controller/commands/deleteCircleCommand.cpp \
    src/controller/commands/deleteCommand.cpp \
    src/controller/commands/deletecursorscommand.cpp \
    src/controller/commands/listCommand.cpp \
    src/controller/commands/listcursorscommand.cpp \
    src/controller/commands/sitrepcommand.cpp \
    src/controller/dclConcController.cpp \
    src/controller/json/jsoncommandhandler.cpp \
    src/controller/json/jsonresponsebuilder.cpp \
    src/controller/json/jsonserializer.cpp \
    src/controller/handlers/cursorcommandhandler.cpp \
    src/controller/handlers/ownshipcommandhandler.cpp \
    src/controller/handlers/geometrycommandhandler.cpp \
    src/controller/services/cursorservice.cpp \
    src/controller/services/geometryservice.cpp \
    src/controller/services/sitrepservice.cpp \
    src/controller/services/trackpppservice.cpp \
    src/controller/services/trackservice.cpp \
    src/controller/services/cpaservice.cpp \
    src/controller/services/ownshipservice.cpp \
    src/controller/services/obmservice.cpp \
    src/controller/services/estacionamientoservice.cpp \
    src/controller/services/queryservice.cpp \
    src/controller/handlers/trackcommandhandler.cpp \
    src/controller/messagerouter.cpp \
    src/controller/overlayHandler.cpp \
    src/controller/json/validators/jsonvalidator.cpp \
    src/main.cpp \
    src/model/cpa.cpp \
    src/model/pppcalculator.cpp \
    src/model/estacionamientocalculator.cpp \
    src/model/decoders/concDecoder.cpp \
    src/model/decoders/lpdEncoder.cpp \
    src/model/entities/cursorEntity.cpp \
    src/model/entities/areaEntity.cpp \
    src/model/entities/circleEntity.cpp \
    src/model/entities/polygonoentity.cpp \
    src/model/network/clientSocket.cpp \
    src/model/network/localipcclient.cpp \
    src/model/network/transportFactory.cpp \
    src/model/network/udpClientAdapter.cpp \
    src/model/obm/obmHandler.cpp \
    src/model/overlays/aaw.cpp \
    src/model/overlays/apc.cpp \
    src/model/overlays/asw.cpp \
    src/model/overlays/ew.cpp \
    src/model/overlays/heco.cpp \
    src/model/overlays/ops.cpp \
    src/model/overlays/spc.cpp \
    src/model/overlays/linco.cpp \
    src/model/entities/track.cpp \
    src/model/ownCursor/owncurs.cpp \
    src/model/sitrep/sitrep.cpp \
    src/model/utils/RadarMath.cpp \
    src/model/utils/configuration.cpp \
    src/view/stdinreader.cpp




