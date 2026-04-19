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
install(TARGETS orVocApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
