cmake_minimum_required(VERSION 2.8.7)
project (multiproj_calib_GUI)

set (SOURCES main.cpp gui_form.cpp)

set (MOC_HEADERS gui_form.h)

set (UIS gui_form.ui)

find_package (Qt4 REQUIRED)
include (${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
include_directories (${CMAKE_BINARY_DIR})
include_directories (${PROJECT_BINARY_DIR})

QT4_WRAP_UI (UI_HEADERS ${UIS})

QT4_WRAP_CPP(MOC_SRC ${MOC_HEADERS})

add_executable (multiproj_calib_GUI ${SOURCES} ${MOC_SRC} ${UI_HEADERS})

target_link_libraries (multiproj_calib_GUI ${QT_LIBRARIES} opencv_calib3d opencv_core opencv_highgui)
