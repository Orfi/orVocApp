# Recommended Directory Structure

VocabBuilder/
├── CMakeLists.txt          # Configured for C++ 17 and Catch2 integration
├── docs/
│   ├── prd.md
│   └── ADR.md
├── src/
│   ├── main.cpp            # QML Engine setup
│   ├── vocabmanager.cpp    # C++ 17 Logic: JSON I/O & Alphabetical Sorting
│   ├── vocabmanager.h
│   ├── networkclient.cpp   # QNetworkAccessManager & API Parsing
│   └── networkclient.h
├── qml/
│   ├── main.qml            # Main Layout
│   ├── Sidebar.qml         # Word List & Search
│   └── TranslationView.qml # RichText display with RTL support
├── tests/
│   ├── main.cpp            # Catch2 Test Runner
│   └── test_logic.cpp      # Unit tests for sorting, parsing, and pathing
└── data/
    └── vocab.json          # Local word bank
