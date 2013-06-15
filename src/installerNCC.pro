#-------------------------------------------------
#
# Project created by QtCreator 2012-12-30T11:26:42
#
#-------------------------------------------------

TARGET = installerNCC
TEMPLATE = lib

contains(QT_VERSION, "^5.*") {
  QT += widgets
}

CONFIG += plugins
CONFIG += dll

DEFINES += INSTALLERNCC_LIBRARY

INCLUDEPATH += "$(BOOSTPATH)"

SOURCES += installerncc.cpp

HEADERS += installerncc.h

LIBS += -lVersion
# -lshell32 -lole32 -luser32 -ladvapi32 -lgdi32 -lPsapi -lVersion

include(../plugin_template.pri)

OTHER_FILES += \
    installerncc.json
