cmake_minimum_required(VERSION 3.21)

project(neurotendrils-app VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_add_executable(neurotendrils-app
    src/main.cpp
    src/theme/ColorPalette.cpp
    src/theme/ThemeManager.cpp
    src/theme/AppFonts.cpp
    src/ui/HomePage.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsMenu.cpp
    src/ui/SettingsOverlay.cpp
    include/theme/ColorPalette.h
    include/theme/ThemeManager.h
    include/theme/AppFonts.h
    include/ui/HomePage.h
    include/ui/MainWindow.h
    include/ui/SettingsMenu.h
    include/ui/SettingsOverlay.h
    resources/resources.qrc
)

target_link_libraries(neurotendrils-app PRIVATE Qt6::Widgets)

target_compile_definitions(neurotendrils-app PRIVATE
    NT_APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(neurotendrils-app PRIVATE include)

set_target_properties(neurotendrils-app PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
