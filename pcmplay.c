/**  PCM rip
 * built with
 * cc pcmplay.c -lportaudio -lrt -lm -lasound -pthread -o pcmplay
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>
#include <stdbool.h>
#include <string.h>

#define FRAMES_PER_BUFFER 256

int sample_rate = 44100;
int channels = 1;
unsigned total_samples = 0;
float *pcm_samples;
unsigned pcm_playback_index = 0;

bool eos = false;

struct
{
    unsigned low_latency : 1;
} flags;

/* Callback function to generate the audio samples */
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    float *out = (float *)outputBuffer;
    unsigned int i;

    for (i = 0; i < framesPerBuffer && pcm_playback_index < total_samples; i++)
    {
        /* Generate the left/mono channel tone (sine wave)*/
        *out++ = pcm_samples[pcm_playback_index++];

        if (channels == 2)
        {
            /* Generate the right/stereo channel tone (sine wave) */
            *out++ = pcm_samples[pcm_playback_index++];
        }
    }
    for (; i < framesPerBuffer; i++)
    {
        *out++ = 0.0f; // Fill remaining frames with silence
        if (channels == 2)
            *out++ = 0.0f;
    }

    if (pcm_playback_index >= total_samples)
    {
        eos = true;
        return paComplete;
    }

    return paContinue;
}

int main()
{
    fread(&sample_rate, sizeof(sample_rate), 1, stdin);
    fread(&channels, sizeof(channels), 1, stdin);
    fread(&total_samples, sizeof(total_samples), 1, stdin);

    pcm_samples = (float *)malloc(total_samples * sizeof(float));
    fread(pcm_samples, sizeof(float), total_samples, stdin);

    PaStream *stream;
    PaError err;
    /* Initialize PortAudio */
    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    /* Open an audio I/O stream */
    err = Pa_OpenDefaultStream(&stream,
                               0,         /* No input channels */
                               channels,  /* 2 output channels (stereo) */
                               paFloat32, /* 32 bit floating point output */
                               sample_rate,
                               FRAMES_PER_BUFFER,
                               patestCallback, /* Callback function */
                               NULL);          /* User data (for phase tracking) */
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    /* Start the audio stream */
    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    while (!eos)
        ;

    /* Stop the audio stream */
    err = Pa_StopStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    /* Close the audio stream */
    err = Pa_CloseStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    /* Terminate PortAudio */
    Pa_Terminate();

    free(pcm_samples);

    return 0;
}
