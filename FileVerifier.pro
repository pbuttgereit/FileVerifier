QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileVerifier
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

VERSION = 0.2.2
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

RC_ICONS = $$PWD/media/FileVerifier.ico

RESOURCES = FileVerifier.qrc

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
		