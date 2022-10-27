#-------------------------------------------------
#
# Project created by QtCreator 2022-10-27T13:40:15
#
#-------------------------------------------------
QT       += core
QT       -= gui

CONFIG += c++11

TARGET = InteliAtsPwr
TEMPLATE = lib

CONFIG         += plugin

DEFINES += INTELIATSPWR_LIBRARY
DEFINES += STANDARD_METER_PLUGIN
DEFINES += METERPLUGIN_FILETREE



SOURCES += \
    inteliatspwr.cpp \
    inteliatspwrdecoder.cpp

HEADERS += \
    inteliatspwr.h \
    inteliatspwrdecoder.h \
    modbusprocessor4inteliatspwr.h


EXAMPLE_FILES = zbyralko.json

linux-beagleboard-g++:{
    target.path = /opt/matilda/plugin
    INSTALLS += target
}
include(../../../Matilda-units/meter-plugin-shared/meter-plugin-shared/meter-plugin-shared.pri)
include(../../../Matilda-units/matilda-base/type-converter/type-converter.pri)
include(../../../Matilda-units/meter-plugin-shared/modbus-base/modbus-base.pri)

linux-g++{
target.path = /opt/plugin2/
INSTALLS += target
}
