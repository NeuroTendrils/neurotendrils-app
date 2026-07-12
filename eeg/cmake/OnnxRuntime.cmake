# Fetch or locate ONNX Runtime and expose onnxruntime::onnxruntime.
#
# Resolution order:
#   1. ONNXRUNTIME_ROOT (manual override)
#   2. find_package(onnxruntime CONFIG)
#   3. Download official prebuilt release for the host platform
#      (macOS .tgz, Linux .tgz, Windows .zip)

set(ONNXRUNTIME_VERSION "1.20.1" CACHE STRING "ONNX Runtime version used when downloading prebuilts")

function(_eeg_ort_shared_lib root out_var)
    if(WIN32)
        if(EXISTS "${root}/lib/onnxruntime.dll")
            set(${out_var} "${root}/lib/onnxruntime.dll" PARENT_SCOPE)
        elseif(EXISTS "${root}/bin/onnxruntime.dll")
            set(${out_var} "${root}/bin/onnxruntime.dll" PARENT_SCOPE)
        else()
            set(${out_var} "" PARENT_SCOPE)
        endif()
    elseif(APPLE)
        set(${out_var} "${root}/lib/libonnxruntime.dylib" PARENT_SCOPE)
    else()
        set(${out_var} "${root}/lib/libonnxruntime.so" PARENT_SCOPE)
    endif()
endfunction()

function(_eeg_import_onnxruntime root)
    if(NOT EXISTS "${root}/include/onnxruntime_cxx_api.h")
        message(FATAL_ERROR "ONNX Runtime root is missing headers: ${root}")
    endif()

    if(WIN32)
        set(_ort_implib "${root}/lib/onnxruntime.lib")
        if(NOT EXISTS "${_ort_implib}")
            message(FATAL_ERROR "ONNX Runtime import library not found: ${_ort_implib}")
        endif()
        _eeg_ort_shared_lib("${root}" _ort_dll)
        if(NOT _ort_dll)
            message(FATAL_ERROR "ONNX Runtime DLL not found under ${root}")
        endif()
        if(NOT TARGET onnxruntime::onnxruntime)
            add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
            set_target_properties(onnxruntime::onnxruntime PROPERTIES
                IMPORTED_IMPLIB "${_ort_implib}"
                IMPORTED_LOCATION "${_ort_dll}"
                INTERFACE_INCLUDE_DIRECTORIES "${root}/include"
            )
        endif()
    else()
        if(APPLE)
            set(_ort_lib "${root}/lib/libonnxruntime.dylib")
        else()
            set(_ort_lib "${root}/lib/libonnxruntime.so")
        endif()
        if(NOT EXISTS "${_ort_lib}")
            message(FATAL_ERROR "ONNX Runtime library not found: ${_ort_lib}")
        endif()
        if(NOT TARGET onnxruntime::onnxruntime)
            add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
            set_target_properties(onnxruntime::onnxruntime PROPERTIES
                IMPORTED_LOCATION "${_ort_lib}"
                INTERFACE_INCLUDE_DIRECTORIES "${root}/include"
            )
        endif()
    endif()

    set(ONNXRUNTIME_ROOT "${root}" PARENT_SCOPE)
endfunction()

function(ensure_onnxruntime)
    if(TARGET onnxruntime::onnxruntime)
        return()
    endif()

    if(DEFINED ENV{ONNXRUNTIME_ROOT} AND NOT ONNXRUNTIME_ROOT)
        set(ONNXRUNTIME_ROOT "$ENV{ONNXRUNTIME_ROOT}" CACHE PATH "ONNX Runtime install prefix")
    endif()

    if(ONNXRUNTIME_ROOT)
        _eeg_import_onnxruntime("${ONNXRUNTIME_ROOT}")
        return()
    endif()

    find_package(onnxruntime CONFIG QUIET)
    if(onnxruntime_FOUND)
        if(TARGET onnxruntime::onnxruntime)
            return()
        endif()
        if(TARGET onnxruntime)
            add_library(onnxruntime::onnxruntime ALIAS onnxruntime)
            return()
        endif()
    endif()

    if(WIN32)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(_ort_arch "win-arm64")
        else()
            set(_ort_arch "win-x64")
        endif()
        set(_ort_archive_ext "zip")
    elseif(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_ort_arch "osx-arm64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            set(_ort_arch "osx-x86_64")
        else()
            message(FATAL_ERROR "Unsupported macOS processor: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        set(_ort_archive_ext "tgz")
    elseif(UNIX)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(_ort_arch "linux-aarch64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            set(_ort_arch "linux-x64")
        else()
            message(FATAL_ERROR "Unsupported Linux processor: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        set(_ort_archive_ext "tgz")
    else()
        message(FATAL_ERROR "Bundled ONNX Runtime download is not configured for ${CMAKE_SYSTEM_NAME}")
    endif()

    set(_ort_url
        "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-${_ort_arch}-${ONNXRUNTIME_VERSION}.${_ort_archive_ext}"
    )

    include(FetchContent)
    FetchContent_Declare(
        onnxruntime_prebuilt
        URL "${_ort_url}"
    )
    FetchContent_MakeAvailable(onnxruntime_prebuilt)

    set(_ort_root "${onnxruntime_prebuilt_SOURCE_DIR}")
    if(NOT EXISTS "${_ort_root}/include/onnxruntime_cxx_api.h")
        file(GLOB _ort_candidates "${onnxruntime_prebuilt_SOURCE_DIR}/onnxruntime-*")
        if(NOT _ort_candidates)
            message(FATAL_ERROR "Could not locate extracted ONNX Runtime directory under ${onnxruntime_prebuilt_SOURCE_DIR}")
        endif()
        list(GET _ort_candidates 0 _ort_root)
    endif()

    _eeg_import_onnxruntime("${_ort_root}")
    message(STATUS "Using downloaded ONNX Runtime ${_ort_arch} v${ONNXRUNTIME_VERSION} from ${_ort_root}")
endfunction()

# Copy the ORT shared library next to the app binary so Win/Linux/macOS
# releases run without a system install of ONNX Runtime.
function(deploy_onnxruntime target)
    if(NOT ONNXRUNTIME_ROOT)
        return()
    endif()

    _eeg_ort_shared_lib("${ONNXRUNTIME_ROOT}" _ort_shared)
    if(NOT _ort_shared OR NOT EXISTS "${_ort_shared}")
        message(WARNING "ONNX Runtime shared library missing; ${target} may fail at runtime")
        return()
    endif()

    if(APPLE)
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "@executable_path/../Frameworks;${ONNXRUNTIME_ROOT}/lib"
            INSTALL_RPATH "@executable_path/../Frameworks"
        )
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_ort_shared}"
                "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks/"
            COMMENT "Bundling ONNX Runtime dylib into app Frameworks"
        )
        # Official packages ship a versioned dylib; copy companions if present.
        file(GLOB _ort_dylibs "${ONNXRUNTIME_ROOT}/lib/libonnxruntime*.dylib")
        foreach(_dylib IN LISTS _ort_dylibs)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dylib}"
                    "$<TARGET_BUNDLE_DIR:${target}>/Contents/Frameworks/"
            )
        endforeach()
    elseif(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_ort_shared}"
                "$<TARGET_FILE_DIR:${target}>/"
            COMMENT "Copying onnxruntime.dll beside executable"
        )
    else()
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "$ORIGIN;${ONNXRUNTIME_ROOT}/lib"
            INSTALL_RPATH "$ORIGIN"
        )
        file(GLOB _ort_sos "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so*")
        foreach(_so IN LISTS _ort_sos)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_so}"
                    "$<TARGET_FILE_DIR:${target}>/"
            )
        endforeach()
    endif()
endfunction()

function(link_onnxruntime target)
    ensure_onnxruntime()
    target_link_libraries(${target} PRIVATE onnxruntime::onnxruntime)

    get_target_property(_nt_target_type ${target} TYPE)
    if(_nt_target_type STREQUAL "EXECUTABLE")
        deploy_onnxruntime(${target})
    elseif(APPLE AND ONNXRUNTIME_ROOT)
        # Static libs still need a build RPATH hint for dependents in-tree.
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "${ONNXRUNTIME_ROOT}/lib"
        )
    endif()
endfunction()
