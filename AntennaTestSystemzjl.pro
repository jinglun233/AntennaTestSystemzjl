QT       += core gui network printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# QCustomPlot 完整版需要 zlib
win32 {
    LIBS += -lz
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    antennadevicewindow.cpp \
    autotestwindow.cpp \
    customtabbar.cpp \
    customtabwidget.cpp \
    dataprotocol.cpp \
    electronicdevicewindow.cpp \
    instrumentcontrolwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    powervoltagewindow.cpp \
    qcustomplot.cpp \
    ringbuffer.cpp \
    temperatureinfowindow.cpp

HEADERS += \
    antennadevicewindow.h \
    autotestwindow.h \
    customtabbar.h \
    customtabwidget.h \
    dataprotocol.h \
    electronicdevicewindow.h \
    instrumentcontrolwindow.h \
    mainwindow.h \
    powervoltagewindow.h \
    qcustomplot.h \
    ringbuffer.h \
    temperatureinfowindow.h

FORMS += \
    antennadevicewindow.ui \
    autotestwindow.ui \
    electronicdevicewindow.ui \
    instrumentcontrolwindow.ui \
    mainwindow.ui \
    powervoltagewindow.ui \
    temperatureinfowindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
