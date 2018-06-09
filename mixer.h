#ifndef _MIXER_H_
#define _MIXER_H_
#include <vector>


enum LoopType {
    none,
    forward,
    pingpong,
};

struct LoopParams {
    LoopType type;
    int begin;
    int end;

    bool is_off() { return type == LoopType::none; }
    bool is_forward() { return type == LoopType::forward; }
    bool is_pingpong() { return type == LoopType::pingpong; }
    int length() { return end - begin; }
};

struct Sample {
    std::vector<float> wavetable;
};

struct AudioChannel {
    float volume;
    float panning;
    float sample_index;
    float sample_step;
    LoopParams loop;
    const Sample *sample;
    bool is_active;
};

struct StereoSample {
    float left;
    float right;
};

void render_audio(AudioChannel* data, StereoSample* out, int samples_remaining);
#endif
