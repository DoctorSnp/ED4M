CONFIG -= qt

TARGET = ED4M

TEMPLATE = lib
VERSION = 1.0.0.4

DEFINES += ED4M_LIBRARY

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/epk.cpp \
    src/klub.cpp \
    src/private_ED4M.cpp \
    src/radiostation.cpp \
    src/saut.cpp \
    src/ts.cpp \
    src/utils/for_rts.cpp \
    src/utils/utils.cpp \
    src/ED4M.cpp \
    src/ED4M_main.cpp

HEADERS += \
    src/ED4M_global.h \
    ED4M.h \
    src/epk.h \
    src/klub.h \
    src/private_ED4M.h \
    src/radiostation.h \
    src/saut.h \
    src/saut_datatype.h \
    src/shared_code.h \
    src/shared_structs.h \
    src/ts.h \
    src/utils/for_rts.h \
    src/utils/utils.h \
    src/ED4M_datatypes/cab/section1/elements.h \
    src/ED4M.h



win32-msvc*
{
   QMAKE_CXXFLAGS +=   /Zp4 #/MP
}

!isEmpty(target.path): INSTALLS += target
