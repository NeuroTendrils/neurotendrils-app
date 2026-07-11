cmake_minimum_required(VERSION 3.21)

project(NeuroTendrils VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(DEFINED ENV{QT_ROOT_DIR})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT_DIR}")
    find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets
        PATHS "$ENV{QT_ROOT_DIR}/lib/cmake/Qt6"
        NO_DEFAULT_PATH
    )
else()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/StripLegacyAgl.cmake")
nt_strip_legacy_agl()

if(Qt6_VERSION VERSION_LESS "6.8.0")
    message(FATAL_ERROR "Qt ${Qt6_VERSION} found but 6.8+ is required for QStyleHints::setColorScheme")
endif()

qt_add_executable(NeuroTendrils
    src/main.cpp
    src/theme/ColorPalette.cpp
    src/theme/ThemeManager.cpp
    src/theme/AppFonts.cpp
    src/ui/HomePage.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsMenu.cpp
    src/ui/SettingsOverlay.cpp
    include/theme/ColorPalette.hpp
    include/theme/ThemeManager.hpp
    include/theme/AppFonts.hpp
    include/ui/HomePage.hpp
    include/ui/MainWindow.hpp
    include/ui/SettingsMenu.hpp
    include/ui/SettingsOverlay.hpp
    resources/resources.qrc
)

target_link_libraries(NeuroTendrils PRIVATE Qt6::Widgets)

target_compile_definitions(NeuroTendrils PRIVATE
    NT_APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(NeuroTendrils PRIVATE include)

set_target_properties(NeuroTendrils PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "NeuroTendrils"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.neurotendrils.app"
)

if(APPLE)
    set(app_icon "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/AppIcon.icns")
    set_source_files_properties("${app_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(NeuroTendrils PRIVATE "${app_icon}")
    set_target_properties(NeuroTendrils PROPERTIES MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
endif()
