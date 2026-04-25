find_package(Qt6 REQUIRED COMPONENTS Quick Network Multimedia QuickDialogs2 Widgets PrintSupport)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(orVocApp
    main.cpp
    vocabmanager.h
    vocabmanager.cpp
    networkclient.h
    networkclient.cpp
    baseexporter.h
    baseexporter.cpp
    pdfexporter.h
    pdfexporter.cpp
)

qt_add_qml_module(orVocApp
    URI orVocab
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
        NotificationOverlay.qml
        ExportPopup.qml
        ImportPopup.qml
    RESOURCES
        splash.png
        icon.png
        icon_256.png
        icon_128.png
        icon_64.png
        icon_48.png
        icon_32.png
        icon_16.png
)

set_target_properties(orVocApp PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(orVocApp
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Qt6::Multimedia
        Qt6::QuickDialogs2
        Qt6::Widgets
        Qt6::PrintSupport
)

include(GNUInstallDirs)

# Real binary -> /usr/lib/orvocapp/orVocApp (alongside bundled Qt libs)
install(TARGETS orVocApp RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp)

# Wrapper script -> /usr/bin/orvocapp (sets LD_LIBRARY_PATH + QT_PLUGIN_PATH)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/packaging/orvocapp-wrapper.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME orvocapp)

# Desktop entry -> /usr/share/applications/orvocapp.desktop
install(FILES ${CMAKE_SOURCE_DIR}/packaging/orvocapp.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Icons -> hicolor theme
install(FILES icon_16.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/16x16/apps RENAME orvocapp.png)
install(FILES icon_32.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps RENAME orvocapp.png)
install(FILES icon_48.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps RENAME orvocapp.png)
install(FILES icon_64.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps RENAME orvocapp.png)
install(FILES icon_128.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps RENAME orvocapp.png)
install(FILES icon_256.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME orvocapp.png)

# ===== Bundle Qt runtime libraries =====
get_target_property(_qt_core_loc Qt6::Core LOCATION)
get_filename_component(QT_LIB_DIR "${_qt_core_loc}" DIRECTORY)
get_filename_component(QT_ROOT_DIR "${QT_LIB_DIR}" DIRECTORY)
set(QT_PLUGIN_DIR "${QT_ROOT_DIR}/plugins")

# Glob all Qt6 and ICU shared libs — avoids missing transitive deps
file(GLOB QT_BUNDLED_LIBS
    "${QT_LIB_DIR}/libQt6*.so.6"
    "${QT_LIB_DIR}/libicu*.so.*"
)
# Also grab FFmpeg and multimedia support libs bundled with Qt
file(GLOB QT_FFMPEG_LIBS
    "${QT_LIB_DIR}/libav*.so.*"
    "${QT_LIB_DIR}/libsw*.so.*"
    "${QT_LIB_DIR}/libQt6FFmpeg*.so.*"
)
list(APPEND QT_BUNDLED_LIBS ${QT_FFMPEG_LIBS})
foreach(_lib ${QT_BUNDLED_LIBS})
    if(EXISTS "${_lib}")
        get_filename_component(_real_lib "${_lib}" REALPATH)
        get_filename_component(_real_name "${_real_lib}" NAME)
        get_filename_component(_link_name "${_lib}" NAME)
        install(FILES "${_real_lib}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp)
        if(NOT "${_real_name}" STREQUAL "${_link_name}")
            install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                \"${_real_name}\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/orvocapp/${_link_name}\")")
        endif()
    endif()
endforeach()

# Qt platform plugins -> /usr/lib/orvocapp/plugins/platforms/
set(QT_PLATFORM_PLUGINS
    "${QT_PLUGIN_DIR}/platforms/libqxcb.so"
    "${QT_PLUGIN_DIR}/platforms/libqwayland.so"
)
foreach(_plugin ${QT_PLATFORM_PLUGINS})
    if(EXISTS "${_plugin}")
        install(FILES "${_plugin}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/plugins/platforms)
    endif()
endforeach()

# XCB GL integration plugins (required for OpenGL context on X11)
set(QT_XCB_GL_PLUGINS
    "${QT_PLUGIN_DIR}/xcbglintegrations/libqxcb-egl-integration.so"
    "${QT_PLUGIN_DIR}/xcbglintegrations/libqxcb-glx-integration.so"
)
foreach(_plugin ${QT_XCB_GL_PLUGINS})
    if(EXISTS "${_plugin}")
        install(FILES "${_plugin}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/plugins/xcbglintegrations)
    endif()
endforeach()

# TLS plugin (required for HTTPS API calls)
if(EXISTS "${QT_PLUGIN_DIR}/tls/libqopensslbackend.so")
    install(FILES "${QT_PLUGIN_DIR}/tls/libqopensslbackend.so"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/plugins/tls)
endif()

# Multimedia plugins (ffmpeg backend)
if(EXISTS "${QT_PLUGIN_DIR}/multimedia/libffmpegmediaplugin.so")
    install(FILES "${QT_PLUGIN_DIR}/multimedia/libffmpegmediaplugin.so"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/plugins/multimedia)
endif()

# Platform input context plugins
set(QT_INPUT_PLUGINS
    "${QT_PLUGIN_DIR}/platforminputcontexts/libcomposeplatforminputcontextplugin.so"
    "${QT_PLUGIN_DIR}/platforminputcontexts/libibusplatforminputcontextplugin.so"
)
foreach(_plugin ${QT_INPUT_PLUGINS})
    if(EXISTS "${_plugin}")
        install(FILES "${_plugin}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/plugins/platforminputcontexts)
    endif()
endforeach()

# QML modules -> /usr/lib/orvocapp/qml/
set(QT_QML_DIR "${QT_ROOT_DIR}/qml")
set(QT_QML_MODULES
    "QtQuick"
    "QtQuick/Controls"
    "QtQuick/Controls/impl"
    "QtQuick/Controls/Basic"
    "QtQuick/Controls/Basic/impl"
    "QtQuick/Controls/Fusion"
    "QtQuick/Controls/Fusion/impl"
    "QtQuick/Dialogs"
    "QtQuick/Layouts"
    "QtQuick/Templates"
    "QtQuick/Window"
    "QtQml"
    "QtQml/Models"
    "QtQml/WorkerScript"
    "QtMultimedia"
    "Qt/labs/folderlistmodel"
    "Qt/labs/platform"
)
foreach(_mod ${QT_QML_MODULES})
    if(IS_DIRECTORY "${QT_QML_DIR}/${_mod}")
        install(DIRECTORY "${QT_QML_DIR}/${_mod}/"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/orvocapp/qml/${_mod}
                FILES_MATCHING PATTERN "*.so" PATTERN "*.qml" PATTERN "qmldir" PATTERN "*.qmltypes")
    endif()
endforeach()
