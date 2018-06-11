#include "mixer.h"
#include <cmath>
#include <cstring>

inline float lerp(float v1, float v2, float t)
{
    return t * v2 + (1.0 - t) * v1;
}

void render_audio(AudioChannel* data, StereoSample* out, int samples_remaining)
{
    float right_panning = data->panning * 0.5 + 0.5;
    float left_panning = 1.0 - right_panning;
    for (; data->is_active && samples_remaining; out++, samples_remaining--) {
        float sample = data->sample->wavetable[data->sample_index];
        out->left = data->volume * sample * left_panning;
        out->right = data->volume * sample * right_panning;

        data->sample_index += data->sample_step;
        if (data->loop.is_off() && data->sample_index >= data->sample->wavetable.size()) {
            data->disable();
        } else if (data->loop.is_forward() && data->sample_index >= data->loop.end) {
            data->sample_index -= data->loop.length();
        } else if (data->loop.is_pingpong()) {
            if (data->sample_step > 0 && data->sample_index >= data->loop.end) {
                data->sample_index = data->loop.end - (data->sample_index - data->loop.end + 1);
                data->sample_step = -data->sample_step;
            } else if (data->sample_step < 0 && data->sample_index < data->loop.begin) {
                data->sample_index = data->loop.begin + (data->loop.begin - data->sample_index);
                data->sample_step = -data->sample_step;
            }
        }
    }
    // Set any remaining samples to zero (silence)
    std::memset(out, 0, samples_remaining * sizeof(out[0]));
}

void Mixer::render(StereoSample* out, int samples_to_render)
{
    memset(out, 0, samples_to_render * sizeof(out[0]));
    for (auto &c : channels_and_buffers) {
        render_audio(&c.channel, &c.buffer[0], samples_to_render);
        for (int i = 0; i < samples_to_render; i++) {
            out[i].left += c.buffer[i].left;
            out[i].right += c.buffer[i].right;
        }
    }
}
