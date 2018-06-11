#ifndef _MIXER_H_
#define _MIXER_H_
#include <algorithm>
#include <cstdint>
#include <vector>

template <typename T>
inline T clamp(T v, T min_v, T max_v)
{
    return std::max(std::min(max_v, v), min_v);
}

enum LoopType {
    none,
    forward,
    pingpong,
};

struct LoopParams {
    LoopType type = LoopType::none;
    uint32_t begin = 0;
    uint32_t end = 0;

    LoopParams() = default;
    LoopParams(LoopType t, uint32_t b, uint32_t e)
        : type(t)
        , begin(b)
        , end(e)
    {
    }
    bool is_off() { return type == LoopType::none; }
    bool is_forward() { return type == LoopType::forward; }
    bool is_pingpong() { return type == LoopType::pingpong; }
    uint32_t length() { return end - begin; }
};

struct Sample {
    std::vector<float> wavetable;
};

struct AudioChannel {
    static constexpr float volume_mute = 0;
    static constexpr float volume_max = 1.0;

    static constexpr float panning_center = 0;
    static constexpr float panning_full_left = -1.0;
    static constexpr float panning_full_right = 1.0;

    float volume;
    float panning;
    float sample_index;
    float sample_step;
    LoopParams loop;
    const Sample* sample;
    bool is_active;
    const int sample_rate;

    explicit AudioChannel(const int rate)
        : sample_rate(rate)
    {
    }
    void set_volume(const float new_volume)
    {
        volume = clamp(new_volume, volume_mute, volume_max);
    }
    void set_panning(const float new_panning)
    {
        panning = clamp(new_panning, panning_full_left, panning_full_right);
    }
    void set_playback_rate(const float playback_rate)
    {
        if (playback_rate > 0) {
            sample_step = playback_rate / sample_rate;
        }
    }
    void play(const Sample* samp, const LoopParams& loop_params)
    {
        sample = samp;
        loop = loop_params;
        sample_index = 0;
        enable();
    }

    void enable()
    {
        is_active = true;
    }
    void disable()
    {
        is_active = false;
    }
};

struct StereoSample {
    float left;
    float right;
};

struct Mixer {
    struct ChannelAndBuffer {
        AudioChannel channel;
        std::vector<StereoSample> buffer;
        explicit ChannelAndBuffer(int sample_rate, size_t max_size)
            : channel(sample_rate)
            , buffer(max_size, { 0, 0 })
        {
        }
    };
    std::vector<StereoSample> accmum_buffer;
    std::vector<ChannelAndBuffer> channels_and_buffers;
    Mixer(size_t num_channels, int sample_rate, size_t max_size)
        : channels_and_buffers(num_channels,
              ChannelAndBuffer(sample_rate, max_size))
    {
    }
    void render(StereoSample* out, size_t samples_remaining);
    AudioChannel& channel(size_t i) { return channels_and_buffers[i].channel; }
};

void render_audio(AudioChannel* data, StereoSample* out, int samples_remaining);
#endif
