#include "portaudio.h"
#include "mixer.h"
#include <cstdio>
#include <fstream>

#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

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

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;

    render_audio(data, (StereoSample*)outputBuffer, framesPerBuffer);
    return paContinue;
}

/*******************************************************************/
int main(void);
int main(void)
{
    PaStreamParameters outputParameters;
    PaStream* stream;
    PaError err;
    AudioChannel data(SAMPLE_RATE);
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

    data.play(&atomic, {.type = LoopType::pingpong, .begin = 12000, .end = samplesize});
    data.set_volume(AudioChannel::volume_max);
    data.set_panning(AudioChannel::panning_center);
    data.set_playback_rate(11025);

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
