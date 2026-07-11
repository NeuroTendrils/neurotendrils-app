# Qt 6.8.1 still links -framework AGL on macOS, but Apple removed AGL from the
# macOS 26 SDK. Strip it when the framework is not present (Qt 6.8.4+ does this upstream).

function(nt_strip_legacy_agl)
    if(NOT APPLE OR NOT TARGET WrapOpenGL::WrapOpenGL)
        return()
    endif()

    find_library(_NT_AGL_FRAMEWORK AGL)
    if(_NT_AGL_FRAMEWORK)
        return()
    endif()

    get_target_property(_nt_gl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(NOT _nt_gl_libs)
        return()
    endif()

    set(_nt_fixed "")
    list(LENGTH _nt_gl_libs _nt_len)
    set(_nt_i 0)
    while(_nt_i LESS _nt_len)
        list(GET _nt_gl_libs ${_nt_i} _nt_item)
        math(EXPR _nt_i "${_nt_i} + 1")
        if(_nt_item STREQUAL "-framework")
            list(GET _nt_gl_libs ${_nt_i} _nt_fw)
            math(EXPR _nt_i "${_nt_i} + 1")
            if(NOT _nt_fw STREQUAL "AGL")
                list(APPEND _nt_fixed "-framework" "${_nt_fw}")
            endif()
        else()
            list(APPEND _nt_fixed "${_nt_item}")
        endif()
    endwhile()

    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_nt_fixed}"
    )
endfunction()
