#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <portaudio.h>
#include <string.h>

#define SAMPLE_RATE 176400
#define FRAMES_PER_BUFFER 64  // Smaller buffer for lower latency
#define NUM_CHANNELS 1
#define SAMPLE_FORMAT paFloat32
#define QUEUE_SIZE 10  // Number of buffers in the circular queue

#define SECONDS 5

typedef float SAMPLE;

// Circular buffer for thread-safe audio transfer
typedef struct {
    SAMPLE buffer[QUEUE_SIZE][FRAMES_PER_BUFFER * NUM_CHANNELS];
    int writeIndex; // Index for the producer to write
    int readIndex;  // Index for the consumer to read
    int count;      // Number of filled slots
    pthread_mutex_t mutex; // Mutex for synchronizing access
    pthread_cond_t cond;   // Condition variable for signaling
} AudioQueue;

// Initialize the audio queue
AudioQueue audioQueue = {
    .writeIndex = 0,
    .readIndex = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

// Push data to the queue (Producer)
void pushToQueue(const SAMPLE *data) {
    pthread_mutex_lock(&audioQueue.mutex);
    while (audioQueue.count == QUEUE_SIZE) {
        // Wait until there's space in the queue
        pthread_cond_wait(&audioQueue.cond, &audioQueue.mutex);
    }
    memcpy(audioQueue.buffer[audioQueue.writeIndex], data,
           FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(SAMPLE));
    audioQueue.writeIndex = (audioQueue.writeIndex + 1) % QUEUE_SIZE;
    audioQueue.count++;
    pthread_cond_signal(&audioQueue.cond); // Notify the consumer
    pthread_mutex_unlock(&audioQueue.mutex);
}

// Pull data from the queue (Consumer)
int pullFromQueue(SAMPLE *data) {
    pthread_mutex_lock(&audioQueue.mutex);
    while (audioQueue.count == 0) {
        // Wait until there's data in the queue
        pthread_cond_wait(&audioQueue.cond, &audioQueue.mutex);
    }
    memcpy(data, audioQueue.buffer[audioQueue.readIndex],
           FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(SAMPLE));
    audioQueue.readIndex = (audioQueue.readIndex + 1) % QUEUE_SIZE;
    audioQueue.count--;
    pthread_cond_signal(&audioQueue.cond); // Notify the producer
    pthread_mutex_unlock(&audioQueue.mutex);
    return 1;
}

unsigned samples_read = 0;

// PortAudio callback
static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    if (inputBuffer != NULL) {
        // Push the captured audio data into the queue
        pushToQueue((const SAMPLE *)inputBuffer);
        if((samples_read += framesPerBuffer) >= SAMPLE_RATE * NUM_CHANNELS * SECONDS)
            return paComplete;
    }
    return paContinue; // Continue recording
}

// Thread function to handle stdout writing
void *stdoutWriter(void *arg) {
    SAMPLE buffer[FRAMES_PER_BUFFER * NUM_CHANNELS];

    int sample_rate = SAMPLE_RATE;
    int channels = NUM_CHANNELS;
    int total_samples = SAMPLE_RATE * NUM_CHANNELS * SECONDS;

    fwrite(&sample_rate, sizeof(sample_rate), 1, stdout);
    fwrite(&channels, sizeof(channels), 1, stdout);
    fwrite(&total_samples, sizeof(total_samples), 1, stdout);

    while (1) {
        // Pull data from the queue and write to stdout
        pullFromQueue(buffer);
        write(STDOUT_FILENO, buffer, FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(SAMPLE));
    }
    return NULL;
}

int main(void) {
    PaStream *stream;
    PaError err;

    // Set stdout to unbuffered mode for immediate output
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return EXIT_FAILURE;
    }

    // Configure the input stream
    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        fprintf(stderr, "Error: No default input device available.\n");
        Pa_Terminate();
        return EXIT_FAILURE;
    }
    inputParams.channelCount = NUM_CHANNELS;
    inputParams.sampleFormat = SAMPLE_FORMAT;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    // Open the input stream with low latency
    err = Pa_OpenStream(&stream,
                        &inputParams,
                        NULL,           // No output stream
                        SAMPLE_RATE,    // Sample rate
                        FRAMES_PER_BUFFER,  // Frames per buffer
                        paClipOff,      // Disable clipping
                        recordCallback, // Audio capture callback
                        NULL);          // No user data
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    // Start the stdout writer thread
    pthread_t writerThread;
    if (pthread_create(&writerThread, NULL, stdoutWriter, NULL) != 0) {
        fprintf(stderr, "Error: Failed to create writer thread.\n");
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    // Start the PortAudio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Recording started with low latency. Outputting raw PCM to stdout...\n");
    fprintf(stderr, "Press Ctrl+C to stop.\n");

    // Keep the stream active until interrupted
    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(10); // Sleep briefly to keep the stream alive
    }

    // Stop the stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    // Close the stream and terminate PortAudio
    Pa_CloseStream(stream);
    Pa_Terminate();

    fprintf(stderr, "Recording stopped.\n");
    return EXIT_SUCCESS;
}
