QT       += core gui xml serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
    LIBS += -LC:\Qt\5.9.9\mingw53_32\lib\libQt5OpenGL.a -lopengl32
    SOURCES += \
    J2534_win.cpp
    HEADERS += \
    J2534_win.h
    HEADERS += \
    J2534_tactrix_win.h
}
linux {
    SOURCES += \
    J2534_linux.cpp
    HEADERS += \
    J2534_linux.h
    HEADERS += \
    J2534_tatrix_linux.h \
}

SOURCES += \
    calibration_maps.cpp \
    calibration_treewidget.cpp \
    definition_file_convert.cpp \
    ecu_operations.cpp \
    ecu_operations_manual.cpp \
    ecu_operations_nissan.cpp \
    ecu_operations_subaru.cpp \
    file_actions.cpp \
    file_defs_ecuflash.cpp \
    file_defs_romraider.cpp \
    log_operations_ssm.cpp \
    logbox.cpp \
    logvalues.cpp \
    main.cpp \
    mainwindow.cpp \
    menu_actions.cpp \
    serial_port_actions.cpp \
    settings.cpp \
    verticallabel.cpp

HEADERS += \
    calibration_maps.h \
    calibration_treewidget.h \
    definition_file_convert.h \
    ecu_operations.h \
    ecu_operations_manual.h \
    ecu_operations_manual.h \
    ecu_operations_nissan.h \
    ecu_operations_subaru.h \
    file_actions.h \
    kernelcomms.h \
    kernelmemorymodels.h \
    logbox.h \
    mainwindow.h \
    serial_port_actions.h \
    settings.h \
    verticallabel.h

FORMS += \
    calibrationmaptable.ui \
    definitionfileconvertwindow.ui \
    ecumanualoperationswindow.ui \
    ecuoperationswindow.ui \
    logvalues.ui \
    mainwindow.ui \
    settings.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc

DISTFILES += \
    LICENSE \
    README.md \
    USING.txt \
    fastecu.rc \
    precompiled/FastECU-Linux.zip \
    precompiled/FastECU-Win7-32bit.zip \
    precompiled/FastECU-Win7-64bit.zip

RC_FILE = fastecu.rc
