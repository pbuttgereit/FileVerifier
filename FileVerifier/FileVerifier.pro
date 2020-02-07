QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileVerifier
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

VERSION = 0.2.0.1

RC_ICONS = FileVerifier.ico

SOURCES += \
        comparereport.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        comparereport.h \
        mainwindow.h

FORMS += \
        comparereport.ui \
        mainwindow.ui
