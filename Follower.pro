QT       += core gui
QT       += qml
QT       += winextras
QT       += concurrent
QT       += network
QT       += xml
#QT       += multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TableEditor/cmdeditor.cpp \
    TableEditor/inputmethodeditor.cpp \
    TableEditor/noteeditor.cpp \
    TableEditor/tablebase.cpp \
    TableEditor/tableeditor.cpp \
    Utils/AudioDevice.cpp \
    Utils/WinUtility.cpp \
    Utils/cacheiconprovider.cpp \
    Utils/gaptimer.cpp \
    Utils/hook.cpp \
    Utils/inputmethod.cpp \
    Utils/keystate.cpp \
    Utils/logfilehandler.cpp \
    Utils/path.cpp \
    Utils/request.cpp \
    Utils/sniper.cpp \
    Utils/systemapi.cpp \
    Utils/timeclipboard.cpp \
    Utils/uiautomation.cpp \
    Utils/util.cpp \
    cmdlistwidget.cpp \
    codeeditor.cpp \
    executor.cpp \
    main.cpp \
    powersettingdia.cpp \
    shortcutdia.cpp \
    toolmenu.cpp \
    updateform.cpp \
    widget.cpp

HEADERS += \
    TableEditor/cmdeditor.h \
    TableEditor/inputmethodeditor.h \
    TableEditor/noteeditor.h \
    TableEditor/tablebase.h \
    TableEditor/tableeditor.h \
    Utils/AudioDevice.h \
    Utils/PolicyConfig.h \
    Utils/WinUtility.h \
    Utils/cacheiconprovider.h \
    Utils/gaptimer.h \
    Utils/hook.h \
    Utils/inputmethod.h \
    Utils/keystate.h \
    Utils/logfilehandler.h \
    Utils/path.h \
    Utils/request.h \
    Utils/sniper.h \
    Utils/systemapi.h \
    Utils/timeclipboard.h \
    Utils/uiautomation.h \
    Utils/util.h \
    cmdlistwidget.h \
    codeeditor.h \
    executor.h \
    powersettingdia.h \
    shortcutdia.h \
    toolmenu.h \
    updateform.h \
    widget.h

FORMS += \
    Utils/sniper.ui \
    powersettingdia.ui \
    shortcutdia.ui \
    tablebase.ui \
    toolmenu.ui \
    updateform.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Function-List.txt \
    Log.txt \
    Need-to-Update.txt

#RC_FILE += icon.rc
RC_ICONS = images/ICON.ico

RESOURCES += \
    Res.qrc

LIBS += -lpsapi -luser32 -lWinmm -lole32 -lPropsys


msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

# 会自动在build目录下生成.rc文件并链接
# 版本
VERSION = 2.16.5
# 公司名称
QMAKE_TARGET_COMPANY = "MrBeanC"
# 文件说明
QMAKE_TARGET_DESCRIPTION = "Follower 2.0"
