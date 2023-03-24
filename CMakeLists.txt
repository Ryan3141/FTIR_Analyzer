cmake_minimum_required(VERSION 3.0.0)
project(FTIR_Analyzer VERSION 0.1.0)
set(CMAKE_TOOLCHAIN_FILE /home/ryan/Code/vcpkg/scripts/buildsystems/vcpkg.cmake)
include(/home/ryan/Code/vcpkg/scripts/buildsystems/vcpkg.cmake)
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ~/Code/vcpkg/installed/x64-linux)
set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_FIND_DEBUG_MODE ON)

include(CTest)
enable_testing()

set(main FTIR_Analyzer)
find_package(libmysql REQUIRED)

# add_compile_definitions( QCUSTOMPLOT_USE_OPENGL )
# Add the include directories for the MySQL C++ Connector library
include_directories(${MySQL_INCLUDE_DIRS})

set(SOURCES base64pp.cpp Ceres_Curve_Fitting.cpp CV_Interactive_Graph.cpp CV_Plotter.cpp CV_Theoretical.cpp Fit_DoubleSpinBox.cpp FTIR_Analyzer.cpp FTIR_Interactive_Graph.cpp Graph_Customizer.cpp HgCdTe.cpp III_V_Materials.cpp Interactive_Graph.cpp Interactive_Graph_Toolbar.cpp IV_By_Device_Size_Plot.cpp IV_By_Size_Plotter.cpp IV_Interactive_Graph.cpp IV_Plotter.cpp Layer_Builder.cpp Lifetime_Interactive_Graph.cpp Lifetime_Plotter.cpp main.cpp Material_Layer_Widget.cpp Optimize.cpp qcustomplot.cpp Report_Plots.cpp SPA_File.cpp SQL_Manager.cpp SQL_Tree_Widget.cpp Thin_Film_Interference.cpp Web_Listener.cpp )
# set(SOURCES base64pp.cpp Ceres_Curve_Fitting.cpp CppAD_Curve_Fitting.cpp CV_Interactive_Graph.cpp CV_Plotter.cpp CV_Theoretical.cpp Fit_DoubleSpinBox.cpp FTIR_Analyzer.cpp FTIR_Interactive_Graph.cpp Graph_Customizer.cpp HgCdTe.cpp III_V_Materials.cpp Interactive_Graph_Toolbar.cpp IV_By_Device_Size_Plot.cpp IV_By_Size_Plotter.cpp IV_Interactive_Graph.cpp IV_Plotter.cpp Layer_Builder.cpp Lifetime_Interactive_Graph.cpp Lifetime_Plotter.cpp main.cpp Material_Layer_Widget.cpp Optimize.cpp qcustomplot.cpp Report_Plots.cpp SPA_File.cpp SQL_Manager.cpp SQL_Tree_Widget.cpp Thin_Film_Interference.cpp )
set(UI_FILES CV_Plotter.ui FTIR_Analyzer.ui Interactive_Graph_Toolbar.ui IV_By_Size_Plotter.ui IV_Plotter.ui Lifetime_Plotter.ui Main.ui Material_Layer_Widget.ui Report_Plots.ui)
# add_compile_definitions(QCUSTOMPLOT_USE_OPENGL)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
# set(CMAKE_AUTOUIC ON)
set(Qt5_NO_SYSTEM_PATHS ON)
# set(CMAKE_PREFIX_PATH "/home/ryan/Code/vcpkg/installed/x64-linux/share/qt5")
# set(CMAKE_MODULE_PATH "")

# Set the Qt5_DIR variable to the vcpkg installation path
# set(Qt5_DIR "/home/ryan/Code/vcpkg/installed/x64-linux-dynamic/lib")
find_package(Qt5 COMPONENTS Widgets Core Gui Network Sql PrintSupport REQUIRED)
# find_package(Qt5 COMPONENTS Widgets Core Gui Network Sql PrintSupport REQUIRED NO_PACKAGE_ROOT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_PACKAGE_REGISTRY )
# find_package(Qt5 COMPONENTS Widgets Core Gui Network Sql PrintSupport REQUIRED NO_PACKAGE_ROOT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_PACKAGE_REGISTRY PATHS "/home/ryan/Code/vcpkg/installed/x64-linux/share/qt5" )
# message(STATUS "Qt5_DIR=${Qt5_DIR}")

# cmake_print_variables(PROJECT_SOURCE_DIR)
qt5_wrap_ui(UI_HEADERS ${UI_FILES})
# SET(CMAKE_EXE_LINKER_FLAGS 
#           "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/home/ryan/Code/vcpkg/installed/x64-linux-dynamic/lib")
add_executable(${main} ${SOURCES} ${UI_HEADERS}) 
target_link_libraries(${main} PRIVATE ${MYSQL_LIBRARIES})

target_link_libraries(${main} PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Network Qt5::Sql Qt5::PrintSupport)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${main} PRIVATE fmt::fmt)

# find_package(yaml-cpp REQUIRED HINTS ~/Code/vcpkg/installed/x64-linux/share/yaml-cpp)
find_package(yaml-cpp CONFIG REQUIRED)
# find_package(yaml-cpp CONFIG REQUIRED NO_PACKAGE_ROOT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_PACKAGE_REGISTRY PATHS "/home/ryan/Code/vcpkg/installed/x64-linux/share/qt5")
target_link_libraries(${main} PRIVATE yaml-cpp)
# target_link_libraries(${main} PRIVATE /home/ryan/Code/vcpkg/installed/x64-linux/lib/libyaml-cpp.a)

find_package(Armadillo REQUIRED)
target_include_directories(${main} PRIVATE ${ARMADILLO_INCLUDE_DIRS})
target_link_libraries(${main} PRIVATE ${ARMADILLO_LIBRARIES})

find_package(Ceres CONFIG REQUIRED)
target_link_libraries(${main} PRIVATE Ceres::ceres)

# # find_package(cppad REQUIRED)
# # target_link_libraries(${main} PRIVATE cppad)
# set(main FTIR_Analyzer)
# find_path(CPPAD_INCLUDE_DIR
#             NAMES cppad/cppad.hpp
#             PATH "/home/ryan/Code/vcpkg/installed/x64-linux")
# # find_library(CPPAD_LIBRARY
# #             NAMES libCPPAD_combined
# #             PATH "/home/ryan/Code/vcpkg/installed/x64-linux/lib")
# set(CPPAD_INCLUDE_DIRS ${CPPAD_INCLUDE_DIR})
# # set(CPPAD_LIBRARIES ${CPPAD_LIBRARY})


find_package(Poco REQUIRED Foundation Net)
include_directories(${Poco_INCLUDE_DIRS})
target_link_libraries(${main} PRIVATE ${Poco_LIBRARIES})
