find_package(Qt6 REQUIRED COMPONENTS Quick Network Multimedia QuickDialogs2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(orVocApp
    main.cpp
    vocabmanager.h
    vocabmanager.cpp
    networkclient.h
    networkclient.cpp
)

qt_add_qml_module(orVocApp
    URI orVocab
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
        SplashScreen.qml
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

set(QT_BUNDLED_LIBS
    "${QT_LIB_DIR}/libQt6Core.so.6"
    "${QT_LIB_DIR}/libQt6Gui.so.6"
    "${QT_LIB_DIR}/libQt6Quick.so.6"
    "${QT_LIB_DIR}/libQt6Qml.so.6"
    "${QT_LIB_DIR}/libQt6QmlModels.so.6"
    "${QT_LIB_DIR}/libQt6Network.so.6"
    "${QT_LIB_DIR}/libQt6Multimedia.so.6"
    "${QT_LIB_DIR}/libQt6QuickControls2.so.6"
    "${QT_LIB_DIR}/libQt6QuickDialogs2.so.6"
    "${QT_LIB_DIR}/libQt6QuickDialogs2QuickImpl.so.6"
    "${QT_LIB_DIR}/libQt6QuickDialogs2Utils.so.6"
    "${QT_LIB_DIR}/libQt6QuickLayouts.so.6"
    "${QT_LIB_DIR}/libQt6QuickTemplates2.so.6"
    "${QT_LIB_DIR}/libQt6OpenGL.so.6"
    "${QT_LIB_DIR}/libQt6DBus.so.6"
    "${QT_LIB_DIR}/libQt6XcbQpa.so.6"
    "${QT_LIB_DIR}/libQt6WaylandClient.so.6"
    "${QT_LIB_DIR}/libicui18n.so.73"
    "${QT_LIB_DIR}/libicuuc.so.73"
    "${QT_LIB_DIR}/libicudata.so.73"
)
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
