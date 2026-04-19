find_package(Qt6 REQUIRED COMPONENTS Core Network)
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_placeholder.cpp
    test_vocabmanager.cpp
    test_networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/vocabmanager.h
    ${CMAKE_SOURCE_DIR}/orVocab/networkclient.cpp
    ${CMAKE_SOURCE_DIR}/orVocab/networkclient.h
)

set_target_properties(orVocabTests PROPERTIES AUTOMOC ON)

target_include_directories(orVocabTests PRIVATE ${CMAKE_SOURCE_DIR}/orVocab)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
        Qt6::Core
        Qt6::Network
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
