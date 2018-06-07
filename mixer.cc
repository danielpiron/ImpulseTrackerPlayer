#include "mixer.h"
#include <cmath>

inline float lerp(float v1, float v2, float t)
{
    return t * v2 + (1.0 - t) * v1;
}

void render_audio(AudioChannel* data, StereoSample* out, int samples_remaining)
{
    auto lerp_sample = [](const std::vector<float>& wavetable, float sample_index) {
        int whole_index = static_cast<int>(sample_index);
        int next_index = (whole_index >= wavetable.size() - 1)
            ? whole_index - wavetable.size() + 1
            : whole_index + 1;
        return lerp(wavetable[whole_index], wavetable[next_index],
            sample_index - floor(sample_index));
    };

    float right_panning = data->panning * 0.5 + 0.5;
    float left_panning = 1.0 - right_panning;
    while (data->is_active && samples_remaining--) {
        float sample = lerp_sample(data->sample->wavetable, data->sample_index);
        out->left = data->volume * sample * left_panning;
        out->right = data->volume * sample * right_panning;

        data->sample_index += data->sample_step;
        if (data->loop.type == LoopType::none
            && data->sample_index >= data->sample->wavetable.size()) {
            data->is_active = false;
        } else if (data->loop.type == LoopType::forward
            && data->sample_index >= data->loop.end) {
            data->sample_index -= data->loop.end - data->loop.begin;
        }
        out++;
    }

    while (samples_remaining > 0) {
        out->left = 0;
        out->right = 0;
        out++;
        samples_remaining--;
    }
}
