#-------------------------------------------------
#
# Project created by QtCreator 2013-10-28T09:35:37
#
#-------------------------------------------------

QT       += core gui

QMAKE_CXXFLAGS += -std=c++0x

TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

VERSION = 0.7.0
DEFINES += VERSIONSTR=\\\"$${VERSION}\\\"

unix: !macx {
	TARGET = qtmind
} else {
	TARGET = QtMind
}

TEMPLATE = app


SOURCES += src/main.cpp\
	src/mainwindow.cpp \
	src/peg.cpp \
	src/button.cpp \
	src/pegbox.cpp \
    src/emptybox.cpp \
    src/message.cpp \
    src/pinbox.cpp \
    src/pin.cpp \
    src/preferences.cpp \
    src/solver.cpp \
    src/game.cpp \
    src/gamerules.cpp \
    src/indicator.cpp \
    src/guesselement.cpp \
    src/boardaid.cpp \
    src/boardsounds.cpp \
    src/boardfont.cpp

HEADERS  += src/mainwindow.h \
	src/peg.h \
	src/constants.h \
	src/button.h \
	src/pegbox.h \
    src/emptybox.h \
    src/message.h \
    src/pinbox.h \
    src/pin.h \
    src/preferences.h \
    src/solver.h \
    src/game.h \
    src/appinfo.h \
    src/gamerules.h \
    src/indicator.h \
    src/guesselement.h \
    src/boardaid.h \
    src/boardsounds.h \
    src/boardfont.h

FORMS    += \
    src/preferences.ui \
    src/mainwindow.ui

RESOURCES += \
    resource.qrc

TRANSLATIONS = translations/qtmind_ara.ts \
	translations/qtmind_af.ts \
	translations/qtmind_ar.ts \
	translations/qtmind_ca.ts \
	translations/qtmind_cs.ts \
	translations/qtmind_cz.ts \
	translations/qtmind_de.ts \
	translations/qtmind_en.ts \
	translations/qtmind_es.ts \
	translations/qtmind_fa.ts \
	translations/qtmind_fo.ts \
	translations/qtmind_fr.ts \
	translations/qtmind_it.ts \
	translations/qtmind_ja.ts \
	translations/qtmind_ko.ts \
	translations/qtmind_nl.ts \
	translations/qtmind_pl.ts \
	translations/qtmind_pt_BR.ts \
	translations/qtmind_ru.ts \
	translations/qtmind_sl.ts \
	translations/qtmind_tr.ts \
	translations/qtmind_zh_CN.ts

OTHER_FILES += \
	qtmind.desktop

unix:!macx { # installation on Unix-ish platforms
	isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr
	isEmpty(BIN_DIR):BIN_DIR = $$INSTALL_PREFIX/bin
	isEmpty(DATA_DIR):DATA_DIR = $$INSTALL_PREFIX/share
	isEmpty(ICON_DIR):ICON_DIR = $$DATA_DIR/pixmaps
	isEmpty(DESKTOP_DIR):DESKTOP_DIR = $$DATA_DIR/applications
	isEmpty(TRANSLATIONS_DIR):TRANSLATIONS_DIR = $$DATA_DIR/qtmind/translations

	target.path = $$BIN_DIR
	icon.files = icons/hicolor/128x128/qtmind.png
	icon.path = $$ICON_DIR
	desktop.path = $$DESKTOP_DIR
	desktop.files = qtmind.desktop
	qm.files = translations/*.qm
	qm.path = $$TRANSLATIONS_DIR

	INSTALLS = target icon desktop qm
}

