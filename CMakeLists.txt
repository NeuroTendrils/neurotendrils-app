cmake_minimum_required(VERSION 3.21)

project(NeuroTendrils VERSION 0.2.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(NT_QT_COMPONENTS Widgets Network Quick Quick3D QuickWidgets Svg)

if(DEFINED ENV{QT_ROOT_DIR})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT_DIR}")
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS ${NT_QT_COMPONENTS})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/StripLegacyAgl.cmake")
nt_strip_legacy_agl()

if(Qt6_VERSION VERSION_LESS "6.8.0")
    message(FATAL_ERROR "Qt ${Qt6_VERSION} found but 6.8+ is required for QStyleHints::setColorScheme")
endif()

# In-tree EEG pipeline (eeg/) + ONNX Runtime. Replay bundle ships under
# assets/eeg/v2 (BNCI2014-001 derived; see assets/eeg/v2/ATTRIBUTION.md).
# Override at runtime with NT_EEG_BUNDLE if needed.
option(NT_ENABLE_EEG "Enable Robotic Arm EEG stream replay (ONNX Runtime)" ON)

set(NT_EEG_SOURCES)
set(NT_EEG_HEADERS)
if(NT_ENABLE_EEG)
    add_subdirectory(eeg)
    list(APPEND NT_EEG_SOURCES
        src/eeg/EegBundlePaths.cpp
        src/eeg/EegStreamPlayer.cpp
        src/eeg/GestureMapper.cpp
        src/ui/EegStreamView.cpp
        src/ui/RoboticArmPage.cpp
    )
    list(APPEND NT_EEG_HEADERS
        include/eeg/EegBundlePaths.hpp
        include/eeg/EegStreamPlayer.hpp
        include/eeg/GestureMapper.hpp
        include/ui/EegStreamView.hpp
        include/ui/RoboticArmPage.hpp
    )
endif()

qt_add_executable(NeuroTendrils
    src/main.cpp
    src/data/AppConfig.cpp
    src/data/AppPaths.cpp
    src/arm/RoArmController.cpp
    src/theme/ColorPalette.cpp
    src/theme/ThemeManager.cpp
    src/theme/AppFonts.cpp
    src/ui/HomePage.cpp
    src/ui/MainWindow.cpp
    src/ui/SettingsMenu.cpp
    src/ui/SettingsOverlay.cpp
    src/ui/BrainView.cpp
    src/ui/ArmWorkspace.cpp
    src/ui/EducationPage.cpp
    src/ui/WipPage.cpp
    ${NT_EEG_SOURCES}
    include/data/AppConfig.hpp
    include/data/AppPaths.hpp
    include/arm/RoArmController.hpp
    include/theme/ColorPalette.hpp
    include/theme/ThemeManager.hpp
    include/theme/AppFonts.hpp
    include/ui/HomePage.hpp
    include/ui/MainWindow.hpp
    include/ui/SettingsMenu.hpp
    include/ui/SettingsOverlay.hpp
    include/ui/BrainView.hpp
    include/ui/ArmWorkspace.hpp
    include/ui/EducationPage.hpp
    include/ui/WipPage.hpp
    ${NT_EEG_HEADERS}
    resources/resources.qrc
    resources/qml.qrc
    resources/data.qrc
)

target_link_libraries(NeuroTendrils PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Quick
    Qt6::Quick3D
    Qt6::QuickWidgets
    Qt6::Svg
)

if(NT_ENABLE_EEG)
    target_link_libraries(NeuroTendrils PRIVATE eeg_inference)
    # Static eeg_inference links ORT privately; ensure the app binary gets RPATH + link.
    link_onnxruntime(NeuroTendrils)
    target_compile_definitions(NeuroTendrils PRIVATE NT_ENABLE_EEG=1)
endif()

# Embed QML plugin imports so Quick3D works without macdeployqt/windeployqt
# beside the executable (dev builds, incomplete deploys).
qt_import_qml_plugins(NeuroTendrils)

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

if(WIN32)
    target_sources(NeuroTendrils PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/NeuroTendrils.rc")
endif()

if(APPLE)
    set(app_icon "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/AppIcon.icns")
    set(brain_model "${CMAKE_CURRENT_SOURCE_DIR}/assets/brain/brain.glb")
    set_source_files_properties("${app_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_source_files_properties("${brain_model}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(NeuroTendrils PRIVATE "${app_icon}" "${brain_model}")
    set_target_properties(NeuroTendrils PROPERTIES MACOSX_BUNDLE_ICON_FILE AppIcon.icns)

    # RuntimeLoader needs plugins/assetimporters; copy into the .app so
    # `open NeuroTendrils.app` works without a full macdeployqt pass.
    get_target_property(_qt_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    execute_process(
        COMMAND "${_qt_qmake_exe}" -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE _qt_plugins_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(EXISTS "${_qt_plugins_dir}/assetimporters")
        add_custom_command(TARGET NeuroTendrils POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:NeuroTendrils>/Contents/PlugIns/assetimporters"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_qt_plugins_dir}/assetimporters"
                "$<TARGET_BUNDLE_DIR:NeuroTendrils>/Contents/PlugIns/assetimporters"
            COMMENT "Bundling Qt assetimporters plugin for RuntimeLoader"
        )
    else()
        message(WARNING "Qt assetimporters plugin not found under ${_qt_plugins_dir}")
    endif()

    if(NT_ENABLE_EEG)
        add_custom_command(TARGET NeuroTendrils POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/eeg/v2"
                "$<TARGET_BUNDLE_DIR:NeuroTendrils>/Contents/Resources/eeg/v2"
            COMMENT "Bundling EEG replay export (BNCI2014-001 derived)"
        )
    endif()
else()
    # Keep large assets out of the executable; ship them next to the binary.
    add_custom_command(TARGET NeuroTendrils POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:NeuroTendrils>/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/brain/brain.glb"
            "$<TARGET_FILE_DIR:NeuroTendrils>/resources/brain.glb"
        COMMENT "Copying brain model beside executable"
    )
    if(NT_ENABLE_EEG)
        add_custom_command(TARGET NeuroTendrils POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets/eeg/v2"
                "$<TARGET_FILE_DIR:NeuroTendrils>/resources/eeg/v2"
            COMMENT "Copying EEG replay export beside executable"
        )
    endif()
endif()
