#include "portaudio.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

#define NUM_SECONDS (5)
#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)
#define TABLE_SIZE (200)

#ifndef M_PI
#define M_PI (3.14159265)
#endif

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

inline float lerp(float v1, float v2, float t)
{
    return t * v2 + (1.0 - t) * v1;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    auto* data = (AudioChannel*)userData;
    StereoSample* out = (StereoSample*)outputBuffer;
    int samples_remaining = framesPerBuffer;

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;

    float right_panning = data->panning * 0.5 + 0.5;
    float left_panning = 1.0 - right_panning;
    while (data->is_active && samples_remaining--) {
        int whole_index = static_cast<int>(data->sample_index);
        int next_index = (whole_index >= data->sample->wavetable.size() - 1)
            ? whole_index - data->sample->wavetable.size() + 1
            : whole_index + 1;
        float sample = lerp(data->sample->wavetable[whole_index],
            data->sample->wavetable[next_index],
            data->sample_index - floor(data->sample_index));
        sample *= data->volume;
        out->left = sample * left_panning;
        out->right = sample * right_panning;
        data->sample_index += data->sample_step;
        if (data->loop.type == LoopType::none
            && data->sample_index >= data->sample->wavetable.size()) {
            data->is_active = false;
        }
        else if (data->loop.type == LoopType::forward
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

    return paContinue;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    AudioChannel data;
    Sample atomic;
    int i;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    std::streampos begin, end;
    std::ifstream rawsample("untitled.raw", std::ios::binary);
    begin = rawsample.tellg();
    rawsample.seekg(0, std::ios::end);
    end = rawsample.tellg();
    auto samplesize = end - begin;
    rawsample.seekg(0, std::ios::beg);

    atomic.wavetable.reserve(samplesize);
    for (size_t i = 0; i < samplesize; i++)
        atomic.wavetable.push_back(static_cast<char>(rawsample.get()) / 127.0);


    data.volume = 1.0; // Full volume
    data.panning = 0; // Center panning
    data.loop.type = LoopType::none;
    data.sample_index = 0;
    data.sample_step = 11025.0 / SAMPLE_RATE;
    data.sample = &atomic;
    data.is_active = true;

    err = Pa_Initialize();
    if (err != paNoError)
        goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2; /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff, /* we won't output out of range samples so don't bother clipping them */
        patestCallback,
        &data);
    if (err != paNoError)
        goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto error;

    printf("Play for %d seconds.\n", NUM_SECONDS);
    Pa_Sleep(NUM_SECONDS * 1000);

    err = Pa_StopStream(stream);
    if (err != paNoError)
        goto error;

    err = Pa_CloseStream(stream);
    if (err != paNoError)
        goto error;

    Pa_Terminate();
    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
