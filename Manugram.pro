#-------------------------------------------------
#
# Project created by QtCreator 2014-10-23T19:17:05
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11 console
QMAKE_CXXFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Manugram
TEMPLATE = app


SOURCES += \
        mainwindow.cpp \
        model.cpp \
    modelwidget.cpp \
    recognition.cpp \
    layouting.cpp \
    model_io.cpp \
    figurepainter.cpp

CONFIG(tests) {
    QT += testlib
    CONFIG += testlib
    SOURCES += tests.cpp
} else {
    SOURCES += main.cpp
}

HEADERS  += mainwindow.h \
            model.h \
    modelwidget.h \
    figurepainter.h \
    recognition.h \
    layouting.h \
    model_io.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
