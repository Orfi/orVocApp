# Product Requirements Document: VocabBuilder Desktop

## 1. Vision
A lightweight, high-performance C++/QML desktop application for personal vocabulary building. It allows users to curate a private "word bank" with instant lookup of definitions, audio pronunciations, and Arabic translations.

## 2. Platform & Portability
- **OS**: Windows 10/11 and Linux (Ubuntu/Mint).
- **Build System**: CMake 3.16+.
- **Compiler**: C++ 17 Standard enforced (MSVC 2017+ / GCC 7+).

## 3. Core Functional Requirements
- **Word Management**: 
    - Input words via a modern QML search/add field.
    - Save words to a local `vocab.json` file with automatic alphabetical sorting.
- **Lookup Engine (REST APIs)**:
    - **English Dictionary**: `https://api.dictionaryapi.dev/api/v2/entries/en/{word}`.
    - **Arabic Translation**: Google Translate Unofficial (GTX) API: `https://translate.googleapis.com/translate_a/single?client=gtx&sl=en&tl=ar&dt=t&q={word}`.
- **Audio Playback**: 
    - Parse the `phonetics` array from the Dictionary API for a valid `.mp3` URL.
    - Provide a UI trigger to play the pronunciation using `QtMultimedia`.
- **Display**: Concatenated HTML view using QML `RichText` with full **RTL** support for Arabic.

## 4. Technical Constraints & QA
- **Unit Testing**: Mandatory unit testing using **Catch2 (v3)** for logic (Sorting, JSON parsing).
- **Path Handling**: Use `QStandardPaths` for cross-platform data integrity.
- **UTF-8**: Project-wide UTF-8 encoding for Arabic support.

## 5. Use Case
The user should be able to Add a new word to a list that is synced with the json file, that has only a list of words.
The List has words sorted in alphabetical order. 
The user should start searching for an existing word in the list using a "Like" approach to narrow down the list which is 
updated in realtime. when double clicking on a word in the list its translation should appear in the controls.
If the word does not exists the user can add it to the list and display its translation. 
The user can play the pronunciation of the word using a play button. 
The user should be able to export the words list json file using an export button.

## 6. GUI design
Wireframe design can be found at design/gui.png

## 7. ADR document
ADR or Architectural Decision Record document can be found at design/ADR.md

## 8. Recommended Directory Structure
can be found at desing/recommended_directory_structure.md 

