#-------------------------------------------------
#
# Project created by QtCreator 2013-08-22T22:50:32
#
#-------------------------------------------------

QT       -= core gui

TARGET = BossDummy
TEMPLATE = lib

DEFINES += BOSSDUMMY_LIBRARY

SOURCES += bossdummy.cpp

HEADERS += bossdummy.h


CONFIG(debug, debug|release) {
  SRCDIR = $$OUT_PWD/debug
  DSTDIR = $$PWD/../../outputd
} else {
  SRCDIR = $$OUT_PWD/release
  DSTDIR = $$PWD/../../output
}

SRCDIR ~= s,/,$$QMAKE_DIR_SEP,g
DSTDIR ~= s,/,$$QMAKE_DIR_SEP,g

PUBLISHSCRIPT = $$quote(powershell.exe -executionpolicy bypass -command $$PWD\\..\\NCC\\publish.ps1)
Debug:    PUBLISHCMD = $$quote($$PUBLISHSCRIPT -debug)
Release:  PUBLISHCMD = $$quote($$PUBLISHSCRIPT -release)

QMAKE_CXXFLAGS_RELEASE -= -MD
QMAKE_CXXFLAGS_RELEASE += -MT

#This sort of dies
#QMAKE_POST_LINK += $$quote($$PUBLISHCMD) $$escape_expand(\\n)
QMAKE_POST_LINK += copy $$quote($$SRCDIR\\BossDummy.dll) $$quote($$DSTDIR\\NCC\\data\\boss32.dll) $$escape_expand(\\n)

OTHER_FILES +=\
    SConscript
