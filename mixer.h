#ifndef _MIXER_H_
#define _MIXER_H_
#include <vector>

enum LoopType {
    none,
    forward,
};

struct LoopParams {
    LoopType type;
    int begin;
    int end;
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
