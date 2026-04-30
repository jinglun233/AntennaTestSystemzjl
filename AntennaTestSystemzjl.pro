QT       += core gui network printsupport axcontainer

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# QCustomPlot 完整版需要 zlib
win32 {
    LIBS += -lz
}

# ========== CsvReaderLib 第三方库 ==========
INCLUDEPATH += $$PWD/3rdparty/csvreaderlib
LIBS += -L$$PWD/3rdparty/csvreaderlib -lCsvReaderLib

# ========== 子目录头文件搜索路径（支持跨目录 #include）==========
INCLUDEPATH += $$PWD/protocol
INCLUDEPATH += $$PWD/network
INCLUDEPATH += $$PWD/device
INCLUDEPATH += $$PWD/widget

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the deprecated APIs.

# ==================== 源文件（按子目录组织）====================

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    protocol/clientprotocol.cpp \
    protocol/serverprotocol.cpp \
    network/networkmanager.cpp \
    network/ringbuffer.cpp \
    device/antennadevicewindow.cpp \
    device/electronicdevicewindow.cpp \
    device/temperatureinfowindow.cpp \
    device/powervoltagewindow.cpp \
    device/autotestwindow.cpp \
    widget/customtabbar.cpp \
    widget/customtabwidget.cpp \
    qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    protocol/clientprotocol.h \
    protocol/serverprotocol.h \
    network/networkmanager.h \
    network/ringbuffer.h \
    device/antennadevicewindow.h \
    device/electronicdevicewindow.h \
    device/temperatureinfowindow.h \
    device/powervoltagewindow.h \
    device/autotestwindow.h \
    widget/customtabbar.h \
    widget/customtabwidget.h \
    qcustomplot.h

FORMS += \
    antennadevicewindow.ui \
    autotestwindow.ui \
    electronicdevicewindow.ui \
    mainwindow.ui \
    powervoltagewindow.ui \
    temperatureinfowindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}.bin
else: unix:!android: target.path = /opt/$${TARGET}.bin
!isEmpty(target.path): INSTALLS = target

# ========== 构建后自动复制 CsvReaderLib DLL 到输出目录 ==========
win32 {
    CSVREADER_DLL = $$PWD/3rdparty/csvreaderlib/CsvReaderLib.dll
    CSVREADER_DEST = $$OUT_PWD/CsvReaderLib.dll
    QMAKE_POST_LINK += $${QMAKE_COPY_FILE} \"$${CSVREADER_DLL}\" \"$${CSVREADER_DEST}\"
}
