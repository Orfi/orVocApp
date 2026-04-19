# tests/CMakeLists.txt
find_package(Catch2 3 REQUIRED)

add_executable(orVocabTests
    test_placeholder.cpp
)

target_link_libraries(orVocabTests
    PRIVATE
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(orVocabTests)
