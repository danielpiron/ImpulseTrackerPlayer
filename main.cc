#include "mixer.h"
#include "portaudio.h"
#include <cstdio>
#include <fstream>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (1024)

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
    auto* mix = reinterpret_cast<Mixer*>(userData);
    StereoSample* out = reinterpret_cast<StereoSample*>(outputBuffer);

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;

    mix->render(out, framesPerBuffer);
    return paContinue;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    Mixer mix(3, SAMPLE_RATE, FRAMES_PER_BUFFER);
    Sample atomic;

    printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    std::ifstream rawsample("untitled.raw", std::ios::binary);
    auto begin = rawsample.tellg();
    rawsample.seekg(0, std::ios::end);
    auto end = rawsample.tellg();
    auto samplesize = end - begin;
    rawsample.seekg(0, std::ios::beg);

    atomic.wavetable.reserve(static_cast<size_t>(samplesize));
    for (int i = 0; i < samplesize; i++)
        atomic.wavetable.push_back(static_cast<char>(rawsample.get()) / 127.0f);

    mix.channel(0).play(&atomic, LoopParams(LoopType::pingpong, 12000, samplesize));
    mix.channel(0).set_volume(AudioChannel::volume_max);
    mix.channel(0).set_panning(AudioChannel::panning_full_left * 0.75f);
    mix.channel(0).set_playback_rate(11025);

    mix.channel(1).play(&atomic, LoopParams(LoopType::pingpong, 12000, samplesize));
    mix.channel(1).set_volume(AudioChannel::volume_max);
    mix.channel(1).set_panning(AudioChannel::panning_full_right * 0.75f);
    mix.channel(1).set_playback_rate(11025 * 1.10f); // 10% faster than the other channel

    mix.channel(2).play(&atomic, LoopParams(LoopType::pingpong, 12000, samplesize));
    mix.channel(2).set_volume(AudioChannel::volume_max);
    mix.channel(2).set_panning(AudioChannel::panning_center);
    mix.channel(2).set_playback_rate(11025 * .95f);

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
        &mix);
    if (err != paNoError)
        goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError)
        goto error;

    printf("Press enter to exit...\n");
    getchar();

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
