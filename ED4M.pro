CONFIG -= qt

TARGET = ED4M

TEMPLATE = lib
VERSION = 1.0.0.4

DEFINES += ED4M_LIBRARY

CONFIG += c++11

INCLUDEPATH += src/RTS/

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/ED4M.cpp \
    src/dll_main.cpp \
    src/equipment/brake_395.cpp \
    src/private_ED4M.cpp \
    src/equipment/epk.cpp \
    src/appliances/klub.cpp \
    src/appliances/radiostation.cpp \
    src/appliances/saut.cpp \
    src/RTS/ts.cpp \
    src/utils/utils.cpp


HEADERS += \
    src/ED4M_data.h \
    src/ED4M_global.h \
    src/ED4M.h \
    src/RTS/rts_data.h \
    src/dll_main.h \
    src/equipment/brake_395.h \
    src/equipment/brake_data.h \
    src/private_ED4M.h \
    src/equipment/epk.h \
    src/appliances/klub.h \
    src/appliances/radiostation.h \
    src/appliances/saut.h \
    src/appliances/saut_datatype.h \
    src/shared_structs.h \
    src/RTS/ts.h \
    src/utils/utils.h \
    src/elements.h

win32-msvc*
{
   QMAKE_CXXFLAGS +=   /Zp4 #/MP
}

!isEmpty(target.path): INSTALLS += target
