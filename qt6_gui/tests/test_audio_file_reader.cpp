#include "models/audio_file_reader.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

TEST_CASE("AudioFileReader", "[audio_file_reader]")
{
    AudioFileReader reader("testdata/chirp.wav");

    SECTION("throws on invalid file path")
    {
        REQUIRE_THROWS_MATCHES(
          AudioFileReader("non_existent_file.wav"),
          std::runtime_error,
          Catch::Matchers::Message("Failed to open audio file non_existent_file.wav for reading: "
                                   "System error : No such file or directory."));
    }

    SECTION("throws on corrupt file")
    {
        REQUIRE_THROWS_MATCHES(
          AudioFileReader("testdata/corrupt.wav"),
          std::runtime_error,
          Catch::Matchers::Message("Failed to open audio file testdata/corrupt.wav for reading: "
                                   "Format not recognised."));
    }

    SECTION("GetSampleRate returns correct value")
    {
        REQUIRE(reader.GetSampleRate() == 44100);
    }

    SECTION("GetChannelCount returns correct value")
    {
        REQUIRE(reader.GetChannelCount() == 1);
    }

    SECTION("GetFrameCount returns correct value")
    {
        REQUIRE(reader.GetFrameCount() == 4410);
    }

    SECTION("reads correct samples")
    {
        const size_t kFramesToRead = 10;
        const auto kHave = reader.ReadInterleaved(kFramesToRead);
        const std::vector<float> kWant = { 0.0f,         0.014247104f, 0.028508738f, 0.042782005f,
                                           0.057063986f, 0.071351744f, 0.08564233f,  0.099932767f,
                                           0.11422006f,  0.128501192f };
        REQUIRE(kHave == kWant);
    }

    SECTION("reads the whole file")
    {
        const auto kHave = reader.ReadInterleaved(10000); // more than total frames
        REQUIRE(kHave.size() == 4410);
    }

    SECTION("reads the whole file in chunks")
    {
        REQUIRE(reader.ReadInterleaved(1000).size() == 1000);
        REQUIRE(reader.ReadInterleaved(1000).size() == 1000);
        REQUIRE(reader.ReadInterleaved(1000).size() == 1000);
        REQUIRE(reader.ReadInterleaved(1000).size() == 1000);
        REQUIRE(reader.ReadInterleaved(1000).size() == 410);
    }
}
