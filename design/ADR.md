# Architecture Decision Record: VocabBuilder

## ADR 001: Tech Stack & Portability
- **Decision**: C++ 17 / Qt 6 (QML) using CMake.
- **Rationale**: Provides a stable foundation using `std::filesystem` and structured bindings while meeting Qt 6 requirements.

## ADR 002: Networking (Pure C++ REST)
- **Decision**: Use `QNetworkAccessManager` (QtNetwork).
- **Rationale**: Bypasses the need for Python wrappers, keeping the application footprint small and native.

## ADR 003: API Response Parsing
- **Decision**: Use `QJsonDocument` for deep-parsing nested API responses.
- **Implementation**: 
    - **Dictionary**: Extract the first valid `.mp3` from the `phonetics` array.
    - **Translation**: Extract the first element of the first array from the Google GTX response.

## ADR 004: Unit Testing Strategy (Catch2)
- **Decision**: Catch2 (v3) for Logic Isolation.
- **Implementation**: Mock `QNetworkReply` to test JSON parsing and verify alphabetical insertion logic in `VocabManager` without requiring live network calls.

## ADR 005: Data Persistence
- **Decision**: `vocab.json` stored via `QStandardPaths::AppDataLocation`.

## ADR 006: Arabic Support
- **Decision**: Inline HTML `dir="rtl"` tags within QML `RichText`.
