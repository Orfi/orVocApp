#include <catch2/catch_all.hpp>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include "networkclient.h"

static int argc = 1;
static char appName[] = "test";
static char *argv[] = {appName, nullptr};

TEST_CASE("NetworkClient dictionary parsing", "[networkclient][dictionary]") {
    QCoreApplication app(argc, argv);

    // Sample dictionary API response for "hello"
    QByteArray dictResponse = R"([
        {
            "word": "hello",
            "phonetic": "/həˈloʊ/",
            "phonetics": [
                { "text": "/həˈloʊ/" },
                { "text": "/həˈloʊ/", "audio": "https://api.dictionaryapi.dev/media/pronunciations/en/hello-us.mp3" }
            ],
            "meanings": [
                {
                    "partOfSpeech": "noun",
                    "definitions": [
                        {
                            "definition": "An utterance of \"hello\"; a greeting.",
                            "example": "she was getting hellos from everyone"
                        }
                    ]
                },
                {
                    "partOfSpeech": "interjection",
                    "definitions": [
                        {
                            "definition": "Used as a greeting."
                        }
                    ]
                }
            ]
        }
    ])";

    SECTION("parseDictionaryResponse extracts formatted HTML") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE_FALSE(result.html.isEmpty());
        REQUIRE(result.html.contains("noun"));
        REQUIRE(result.html.contains("interjection"));
        REQUIRE(result.html.contains("An utterance of"));
        REQUIRE(result.html.contains("she was getting hellos"));
    }

    SECTION("parseDictionaryResponse extracts phonetic text") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.phonetic == "/həˈloʊ/");
    }

    SECTION("parseDictionaryResponse extracts first valid mp3 URL") {
        auto result = NetworkClient::parseDictionaryResponse(dictResponse);
        REQUIRE(result.audioUrl == QUrl("https://api.dictionaryapi.dev/media/pronunciations/en/hello-us.mp3"));
    }

    SECTION("parseDictionaryResponse with no audio returns empty URL") {
        QByteArray noAudio = R"([{
            "word": "test",
            "phonetics": [{ "text": "/tɛst/" }],
            "meanings": [{ "partOfSpeech": "noun", "definitions": [{ "definition": "A trial." }] }]
        }])";

        auto result = NetworkClient::parseDictionaryResponse(noAudio);
        REQUIRE(result.audioUrl.isEmpty());
    }

    SECTION("parseDictionaryResponse with empty data returns error") {
        auto result = NetworkClient::parseDictionaryResponse("{}");
        REQUIRE(result.html.isEmpty());
        REQUIRE(result.error == true);
    }
}
