#-------------------------------------------------
#
# Project created by QtCreator 2012-12-30T11:26:42
#
#-------------------------------------------------

TARGET = installerNCC
TEMPLATE = lib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += plugins
CONFIG += dll

CONFIG(release, debug|release) {
  QMAKE_CXXFLAGS += /Zi
  QMAKE_LFLAGS += /DEBUG
}

DEFINES += INSTALLERNCC_LIBRARY


SOURCES += installerncc.cpp

HEADERS += installerncc.h

LIBS += -lVersion
# -lshell32 -lole32 -luser32 -ladvapi32 -lgdi32 -lPsapi -lVersion

include(../plugin_template.pri)

INCLUDEPATH += "$${BOOSTPATH}"

OTHER_FILES += \
    installerncc.json\
    SConscript
