#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <sample_buffer.h>
#include <thread>
#include <vector>

TEST_CASE("SampleBuffer basic functionality", "[SampleBuffer]")
{
    const size_t kSampleRate = 44100;
    SampleBuffer buffer(kSampleRate);

    std::vector<float> kSamples = { 0.1f, 0.2f, 0.3f, 0.4f };
    buffer.AddSamples(kSamples);

    SECTION("Check properties")
    {
        REQUIRE(buffer.GetSampleRate() == kSampleRate);
        REQUIRE(buffer.NumSamples() == kSamples.size());
    }

    SECTION("Retrieve all samples")
    {
        auto retrieved = buffer.GetSamples(0, kSamples.size());
        REQUIRE(retrieved == kSamples);
    }

    SECTION("Retrieve partial samples")
    {
        auto retrieved = buffer.GetSamples(1, 2);
        REQUIRE(retrieved == std::vector<float>({ 0.2f, 0.3f }));
    }

    SECTION("Retrieve samples zero-padded before start")
    {
        auto retrieved = buffer.GetSamples(-2, 4);
        REQUIRE(retrieved == std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f }));
    }

    SECTION("Retrieve samples zero-padded after end")
    {
        auto retrieved = buffer.GetSamples(2, 4);
        REQUIRE(retrieved == std::vector<float>({ 0.3f, 0.4f, 0.0f, 0.0f }));
    }

    SECTION("Retrieve samples zero-padded on both sides")
    {
        auto retrieved = buffer.GetSamples(-2, 8);
        REQUIRE(retrieved ==
                std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.0f, 0.0f }));
    }

    SECTION("Append more samples")
    {
        std::vector<float> const kNewSamples = { 0.5f, 0.6f };
        buffer.AddSamples(kNewSamples);

        auto const kRetrieved = buffer.GetSamples(0, kSamples.size() + kNewSamples.size());
        REQUIRE(kRetrieved == std::vector<float>({ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f }));
    }
}

TEST_CASE("SampleBuffer concurrent access", "[SampleBuffer][concurrency]")
{
    SampleBuffer buffer(44100);

    auto writer = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            std::vector<float> const kSamples = { static_cast<float>(i) };
            buffer.AddSamples(kSamples);
        }
    };

    auto reader = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            volatile auto samples = buffer.GetSamples(0, buffer.NumSamples());
        }
    };

    std::thread thread1(writer);
    std::thread thread2(reader);
    std::thread thread3(reader);

    thread1.join();
    thread2.join();
    thread3.join();

    REQUIRE(buffer.NumSamples() == 1000);
}