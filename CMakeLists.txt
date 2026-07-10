cmake_minimum_required(VERSION 3.21)

project(neurotendrils-app VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(neurotendrils-app
    src/main.cpp
    src/theme/ColorPalette.cpp
    src/theme/ThemeManager.cpp
    src/ui/MainWindow.cpp
    include/theme/ColorPalette.h
    include/theme/ThemeManager.h
    include/ui/MainWindow.h
)

target_link_libraries(neurotendrils-app PRIVATE Qt6::Widgets)

target_include_directories(neurotendrils-app PRIVATE include)

set_target_properties(neurotendrils-app PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
