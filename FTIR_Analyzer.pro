# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

TEMPLATE = app
TARGET = FTIR_Analyzer
DESTDIR = ../x64/Debug
QT += core sql opengl gui widgets printsupport
CONFIG += debug
DEFINES += GLOG_NO_ABBREVIATED_SEVERITIES QCUSTOMPLOT_USE_OPENGL _UNICODE WIN64 QT_OPENGL_LIB QT_PRINTSUPPORT_LIB QT_SQL_LIB QT_WIDGETS_LIB
INCLUDEPATH += ../../../../../../../../Code/boost_1_67_0 \
    ../../../../../../../../Code/OpenBLAS/OpenBLAS-0.2.18 \
    ../../../../../../../../Code/armadillo-8.600.0/include \
    ./GeneratedFiles \
    . \
    ./GeneratedFiles/$(ConfigurationName)
LIBS += -L"../../../../../../../../Code/armadillo-8.600.0/examples/lib_win64" \
    -lblas_win64_MT \
    -llapack_win64_MT \
    -lopengl32 \
    -lglu32
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$(ConfigurationName)
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(FTIR_Analyzer.pri)