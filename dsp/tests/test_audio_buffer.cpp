#include <audio_buffer.h>
#include <catch2/catch_test_macros.hpp>
#include <thread>

TEST_CASE("AudioBuffer basic functionality", "[AudioBuffer]")
{
    const size_t sample_rate = 44100;
    AudioBuffer buffer(sample_rate);

    std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f };
    buffer.add_samples(samples);

    SECTION("Check properties")
    {
        REQUIRE(buffer.sample_rate() == sample_rate);
        REQUIRE(buffer.numSamples() == samples.size());
    }

    SECTION("Retrieve all samples")
    {
        auto retrieved = buffer.get_samples(0, samples.size());
        REQUIRE(retrieved == samples);
    }

    SECTION("Retrieve partial samples")
    {
        auto retrieved = buffer.get_samples(1, 2);
        REQUIRE(retrieved == std::vector<float>({ 0.2f, 0.3f }));
    }

    SECTION("Retrieve samples zero-padded before start")
    {
        auto retrieved = buffer.get_samples(-2, 4);
        REQUIRE(retrieved == std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f }));
    }

    SECTION("Retrieve samples zero-padded after end")
    {
        auto retrieved = buffer.get_samples(2, 4);
        REQUIRE(retrieved == std::vector<float>({ 0.3f, 0.4f, 0.0f, 0.0f }));
    }

    SECTION("Retrieve samples zero-padded on both sides")
    {
        auto retrieved = buffer.get_samples(-2, 8);
        REQUIRE(retrieved ==
                std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.0f, 0.0f }));
    }

    SECTION("Append more samples")
    {
        std::vector<float> new_samples = { 0.5f, 0.6f };
        buffer.add_samples(new_samples);

        auto retrieved = buffer.get_samples(0, samples.size() + new_samples.size());
        REQUIRE(retrieved == std::vector<float>({ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f }));
    }
}

TEST_CASE("AudioBuffer concurrent access", "[AudioBuffer][concurrency]")
{
    AudioBuffer buffer(44100);

    auto writer = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            std::vector<float> samples = { static_cast<float>(i) };
            buffer.add_samples(samples);
        }
    };

    auto reader = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            volatile auto samples = buffer.get_samples(0, buffer.numSamples());
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    std::thread t3(reader);

    t1.join();
    t2.join();
    t3.join();

    REQUIRE(buffer.numSamples() == 1000);
}