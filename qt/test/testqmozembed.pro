TEMPLATE = app

HEADERS = test.h

SOURCES = test.cpp

TARGET = testqmozembed

CONFIG += qt
debug:CONFIG += console

unix:QMAKE_CXXFLAGS += -fshort-wchar -fno-rtti

INCLUDEPATH += ..

debug:LIBS += -L../debug
release:LIBS += -L../release
unix:LIBS += -L..

LIBS += -lqmozembed


# needed because using static lib for now
QMAKE_CXXFLAGS += -include mozilla-config.h
unix:QMAKE_CXXFLAGS += -fno-short-wchar -std=c++0x -fPIC
DEFINES += XPCOM_GLUE=1 XPCOM_GLUE_USE_NSPR=1 MOZ_GLUE_IN_PROGRAM=1

win32:DEFINES += XP_WIN=1

INCLUDEPATH += $(GRE_HOME)/../include/xulapp \
	$(GRE_HOME)/../include/nspr \
	$(GRE_HOME)/../include/xpcom \
	$(GRE_HOME)/../include/string \
	$(GRE_HOME)/../include/profdirserviceprovider \
	$(GRE_HOME)/../include/embed_base \
	$(GRE_HOME)/../include/windowwatcher \
	$(GRE_HOME)/../include/webbrwsr \
	$(GRE_HOME)/../include/shistory \
	$(GRE_HOME)/../include/uriloader \
	$(GRE_HOME)/../include/dom \
	$(GRE_HOME)/../include/necko \
	$(GRE_HOME)/../include/widget \
	$(GRE_HOME)/../include/docshell \
	$(GRE_HOME)/../include/profile \
	$(GRE_HOME)/../include \
	$(GRE_HOME)/../include/gfx \
	../common

LIBS += -L$(GRE_HOME)/../lib

win32:LIBS += -ladvapi32

LIBS += -lxpcomglue -ldl
LIBS += -Wl,--whole-archive -L$(GRE_HOME)/../lib -lmozglue
LIBS += -Wl,--no-whole-archive -rdynamic

# Copy default mozilla flags to avoid some gcc warnings
*-g++*: QMAKE_CXXFLAGS += -Wno-attributes
*-g++*: QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
*-g++*: QMAKE_CXXFLAGS += -pedantic
*-g++*: QMAKE_CXXFLAGS += -Wall
*-g++*: QMAKE_CXXFLAGS += -Wno-unused-parameter
*-g++*: QMAKE_CXXFLAGS += -Wpointer-arith
*-g++*: QMAKE_CXXFLAGS += -Woverloaded-virtual
*-g++*: QMAKE_CXXFLAGS += -Werror=return-type
*-g++*: QMAKE_CXXFLAGS += -Wtype-limits
*-g++*: QMAKE_CXXFLAGS += -Wempty-body
*-g++*: QMAKE_CXXFLAGS += -Wno-ctor-dtor-privacy
*-g++*: QMAKE_CXXFLAGS += -Wno-overlength-strings
*-g++*: QMAKE_CXXFLAGS += -Wno-invalid-offsetof
*-g++*: QMAKE_CXXFLAGS += -Wno-variadic-macros
*-g++*: QMAKE_CXXFLAGS += -Wno-long-long
*-g++*: QMAKE_CXXFLAGS += -Wno-psabi
