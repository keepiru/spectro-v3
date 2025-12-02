#include <audio_buffer.h>
#include <catch2/catch_test_macros.hpp>
#include <thread>

TEST_CASE("AudioBuffer basic functionality", "[AudioBuffer]")
{
    const size_t channels = 2;
    const size_t sample_rate = 44100;
    AudioBuffer buffer(channels, sample_rate);

    std::vector<float> left_channel = { 0.1f, 0.2f, 0.3f, 0.4f };
    std::vector<float> right_channel = { 0.5f, 0.6f, 0.7f, 0.8f };
    std::vector<std::vector<float>> samples = { left_channel, right_channel };
    buffer.add_frames_planar(samples);

    SECTION("Check properties")
    {
        REQUIRE(buffer.channels() == channels);
        REQUIRE(buffer.sample_rate() == sample_rate);
    }

    SECTION("Retrieve all samples")
    {
        auto retrieved_left = buffer.get_channel_samples(0, 0, left_channel.size());
        auto retrieved_right = buffer.get_channel_samples(1, 0, right_channel.size());

        REQUIRE(retrieved_left == left_channel);
        REQUIRE(retrieved_right == right_channel);
    }

    SECTION("Retrieve partial samples")
    {
        auto retrieved_left = buffer.get_channel_samples(0, 1, 2);
        auto retrieved_right = buffer.get_channel_samples(1, 2, 2);

        REQUIRE(retrieved_left == std::vector<float>({ 0.2f, 0.3f }));
        REQUIRE(retrieved_right == std::vector<float>({ 0.7f, 0.8f }));
    }

    SECTION("Retrieve samples zero-padded before start")
    {
        auto retrieved_left = buffer.get_channel_samples(0, -2, 4);
        auto retrieved_right = buffer.get_channel_samples(1, -2, 4);

        REQUIRE(retrieved_left == std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f }));
        REQUIRE(retrieved_right == std::vector<float>({ 0.0f, 0.0f, 0.5f, 0.6f }));
    }

    SECTION("Retrieve samples zero-padded after end")
    {
        auto retrieved_left = buffer.get_channel_samples(0, 2, 4);
        auto retrieved_right = buffer.get_channel_samples(1, 2, 4);

        REQUIRE(retrieved_left == std::vector<float>({ 0.3f, 0.4f, 0.0f, 0.0f }));
        REQUIRE(retrieved_right == std::vector<float>({ 0.7f, 0.8f, 0.0f, 0.0f }));
    }

    SECTION("Retrieve samples zero-padded on both sides")
    {
        auto retrieved_left = buffer.get_channel_samples(0, -2, 8);
        auto retrieved_right = buffer.get_channel_samples(1, -2, 8);

        REQUIRE(retrieved_left ==
                std::vector<float>({ 0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.0f, 0.0f }));
        REQUIRE(retrieved_right ==
                std::vector<float>({ 0.0f, 0.0f, 0.5f, 0.6f, 0.7f, 0.8f, 0.0f, 0.0f }));
    }

    SECTION("Append more samples")
    {
        std::vector<float> new_left = { 0.9f, 1.0f };
        std::vector<float> new_right = { 1.1f, 1.2f };
        std::vector<std::vector<float>> new_samples = { new_left, new_right };
        buffer.add_frames_planar(new_samples);

        auto retrieved_left =
          buffer.get_channel_samples(0, 0, left_channel.size() + new_left.size());
        auto retrieved_right =
          buffer.get_channel_samples(1, 0, right_channel.size() + new_right.size());

        REQUIRE(retrieved_left == std::vector<float>({ 0.1f, 0.2f, 0.3f, 0.4f, 0.9f, 1.0f }));
        REQUIRE(retrieved_right == std::vector<float>({ 0.5f, 0.6f, 0.7f, 0.8f, 1.1f, 1.2f }));
    }

    SECTION("Throws if channel count mismatch on add")
    {
        std::vector<std::vector<float>> bad_samples = { left_channel }; // only 1 channel
        REQUIRE_THROWS_AS(buffer.add_frames_planar(bad_samples), std::invalid_argument);
    }

    SECTION("Throws if frame count mismatch on add")
    {
        std::vector<float> short_right_channel = { 0.5f, 0.6f }; // only 2 samples
        std::vector<std::vector<float>> bad_samples = { left_channel, short_right_channel };
        REQUIRE_THROWS_AS(buffer.add_frames_planar(bad_samples), std::invalid_argument);
    }
}

TEST_CASE("AudioBuffer concurrent access", "[AudioBuffer][concurrency]")
{
    AudioBuffer buffer(1, 44100);

    auto writer = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            std::vector<std::vector<float>> samples = { { static_cast<float>(i) } };
            buffer.add_frames_planar(samples);
        }
    };

    auto reader = [&buffer]() {
        for (int i = 0; i < 1000; ++i) {
            volatile auto samples = buffer.get_channel_samples(0, 0, buffer.numFrames());
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    std::thread t3(reader);

    t1.join();
    t2.join();
    t3.join();

    REQUIRE(buffer.numFrames() == 1000);
}