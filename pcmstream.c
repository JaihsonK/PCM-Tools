#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define FRAMES_PER_BUFFER 256
#define PCM_BUFFER_SIZE (FRAMES_PER_BUFFER * 10) // Enough to hold 10 chunks of audio

typedef struct {
    float buffer[PCM_BUFFER_SIZE];
    unsigned int readIndex;
    unsigned int writeIndex;
    unsigned int availableFrames;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} PCMBuffer;

PCMBuffer pcmBuffer = {
    .readIndex = 0,
    .writeIndex = 0,
    .availableFrames = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

int sample_rate = 44100;
int channels = 1;
bool eos = false;

/* PCM Producer: Reads data from stdin and fills the PCM buffer */
void *pcmProducer(void *arg) {
    while (!eos) {
        float tempBuffer[FRAMES_PER_BUFFER * channels];
        size_t framesRead = fread(tempBuffer, sizeof(float), FRAMES_PER_BUFFER * channels, stdin);

        if (framesRead == 0) {
            eos = true;
            break;
        }

        pthread_mutex_lock(&pcmBuffer.mutex);

        for (size_t i = 0; i < framesRead; i++) {
            pcmBuffer.buffer[pcmBuffer.writeIndex] = tempBuffer[i];
            pcmBuffer.writeIndex = (pcmBuffer.writeIndex + 1) % PCM_BUFFER_SIZE;
            //fprintf(stdout, "\n%f", tempBuffer[i]);
        }

        pcmBuffer.availableFrames += framesRead / channels;
        pthread_cond_signal(&pcmBuffer.cond);
        pthread_mutex_unlock(&pcmBuffer.mutex);
    }
    return NULL;
}

/* PCM Consumer: PortAudio callback function */
static int patestCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    float *out = (float *)outputBuffer;

    pthread_mutex_lock(&pcmBuffer.mutex);
    while (pcmBuffer.availableFrames < framesPerBuffer && !eos) {
        pthread_cond_wait(&pcmBuffer.cond, &pcmBuffer.mutex);
    }

    unsigned int framesToProcess = framesPerBuffer;
    if (pcmBuffer.availableFrames < framesPerBuffer) {
        framesToProcess = pcmBuffer.availableFrames;
    }

    for (unsigned int i = 0; i < framesToProcess * channels; i++) {
        *out++ = pcmBuffer.buffer[pcmBuffer.readIndex];
        pcmBuffer.readIndex = (pcmBuffer.readIndex + 1) % PCM_BUFFER_SIZE;
    }

    pcmBuffer.availableFrames -= framesToProcess;

    // Fill remaining buffer space with silence if EOS
    for (unsigned int i = framesToProcess; i < framesPerBuffer; i++) {
        *out++ = 0.0f;
        if (channels == 2) *out++ = 0.0f;
    }

    pthread_mutex_unlock(&pcmBuffer.mutex);

    if (eos && pcmBuffer.availableFrames == 0) {
        return paComplete;
    }

    return paContinue;
}

int main() {
    PaStream *stream;
    PaError err;

    fread(&sample_rate, sizeof(sample_rate), 1, stdin);
    fread(&channels, sizeof(channels), 1, stdin);
    fread(&err /*just to advance file pointer*/, sizeof(int), 1, stdin);

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // Open audio stream
    err = Pa_OpenDefaultStream(&stream,
                               0,         /* No input channels */
                               channels,  /* Output channels */
                               paFloat32, /* 32-bit floating point output */
                               sample_rate,
                               FRAMES_PER_BUFFER,
                               patestCallback, /* Callback function */
                               NULL);          /* No user data */
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    // Create PCM producer thread
    pthread_t producerThread;
    if (pthread_create(&producerThread, NULL, pcmProducer, NULL) != 0) {
        fprintf(stderr, "Error: Failed to create PCM producer thread.\n");
        Pa_Terminate();
        return 1;
    }

    // Start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    // Wait for stream to finish
    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(10);
    }

    // Stop audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    // Close audio stream
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    // Wait for producer thread to finish
    pthread_join(producerThread, NULL);

    // Terminate PortAudio
    Pa_Terminate();

    return 0;
}
