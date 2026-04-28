find_package(Qt6 REQUIRED COMPONENTS Core Network Gui Widgets PrintSupport)

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

add_executable(orVocAppTests
    test_vocabmanager.cpp
    test_networkclient.cpp
    test_pdfexporter.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/baseexporter.cpp
    ${CMAKE_SOURCE_DIR}/orVocApp/pdfexporter.cpp
)

set_target_properties(orVocAppTests PROPERTIES AUTOMOC ON)

target_include_directories(orVocAppTests PRIVATE ${CMAKE_SOURCE_DIR}/orVocApp)

target_link_libraries(orVocAppTests
    PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
        Qt6::Network
        Qt6::Gui
        Qt6::Widgets
        Qt6::PrintSupport
)

include(CTest)
include(Catch)
catch_discover_tests(orVocAppTests
    DL_PATHS "$<TARGET_FILE_DIR:Qt6::Core>"
)
