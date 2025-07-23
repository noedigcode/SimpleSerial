# Add this to your Qt project .pro file, e.g.:
# 
#   include(../path/to/gidqt5serial.pri)
#
# By including this .pri file, the following is automatically done:

#   - The serialport module is added to QT, which is required to use QSerialPort.

#   - The GidQt5Serial header, source and form files are added to your project
#     so they will be included in your build.

#   - The GidQt5Serial directory is added to INCLUDEPATH so the header file can
#     simply be included in your source files like:
#   
#       #include "gidqt5serial.h"
# 


QT += serialport

GIDQT5SERIAL_DIR = $$PWD

INCLUDEPATH += $${GIDQT5SERIAL_DIR}

SOURCES += \
    $${GIDQT5SERIAL_DIR}/gidqt5serial.cpp

HEADERS += \
    $${GIDQT5SERIAL_DIR}/gidqt5serial.h

FORMS += \
    $${GIDQT5SERIAL_DIR}/gidqt5serial.ui
