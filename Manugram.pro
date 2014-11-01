#-------------------------------------------------
#
# Project created by QtCreator 2014-10-23T19:17:05
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11 console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Manugram
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        model.cpp \
    modelwidget.cpp \
    recognition.cpp

HEADERS  += mainwindow.h \
            model.h \
    modelwidget.h \
    figurepainter.h \
    recognition.h

FORMS    += mainwindow.ui
