#include <stdio.h>
#include <stdlib.h>
#include <portaudio.h>
#include <time.h>

#define FRAMES_PER_BUFFER 64
#define BUFFER_SIZE (FRAMES_PER_BUFFER * 256) // Larger buffer for safety
#define CHANNELS 1
#define SAMPLE_RATE 384000

float *buffer;
unsigned long r_index = 0, w_index = 0;

void clean()
{
    if (buffer)
    {
        free(buffer);
    }
}

float speed_of_sound(float temp)
{
    return (331 + 0.6*temp) * 100; //cm
}

int latency(float distance)
{
    //Given a certain distance from the mic, what is the needed latency to cancel sound waves

    int frames_per_ms = SAMPLE_RATE / 1000;
    float speed_of_sound_per_ms = speed_of_sound(22.3f) / 1000.0f;

    return distance / speed_of_sound_per_ms * frames_per_ms;
}

int pa_callback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    const float *finput = (const float *)input;
    float *foutput = (float *)output;

    static struct timespec input_time = {0};  // Time of first input
    static struct timespec output_time = {0}; // Time of first output
    static int input_marked = 0;              // Flag for marking first input
    static int output_marked = 0;             // Flag for marking first output

    for (unsigned long i = 0; i < frameCount; i++)
    {
        // Write input data to the buffer
        buffer[w_index++ % BUFFER_SIZE] = finput ? finput[i] : 0.0f;

        // if (!input_marked) // Mark time of first input
        // {
        //     clock_gettime(CLOCK_REALTIME, &input_time);
        //     input_marked = 1;
        // }

        // Read data from the buffer for output
        if ((w_index - r_index) >= latency(18.0) || 1)
        {

            // if (!output_marked) // Mark time of first output
            // {
            //     clock_gettime(CLOCK_REALTIME, &output_time);
            //     output_marked = 1;

            //     // Calculate time difference in seconds and nanoseconds
            //     double diff_sec = output_time.tv_sec - input_time.tv_sec;
            //     double diff_nsec = (output_time.tv_nsec - input_time.tv_nsec) / 1e9;
            //     double total_diff = diff_sec + diff_nsec;

            //     printf("Time difference between input and output: %.9f seconds\n", total_diff);
            // }
            float sample = buffer[r_index++ % BUFFER_SIZE];

            // Write to output (mono or stereo)
            if (CHANNELS == 1)
            {
                foutput[i] = -sample;
            }
            else if (CHANNELS == 2)
            {
                foutput[i * 2] = -sample;     // Left
                foutput[i * 2 + 1] = -sample; // Right
            }
        }
        else
        {
            // Output silence if no data is ready
            if (CHANNELS == 1)
            {
                foutput[i] = 0.0f;
            }
            else if (CHANNELS == 2)
            {
                foutput[i * 2] = 0.0f;
                foutput[i * 2 + 1] = 0.0f;
            }
        }
    }

    return paContinue;
}

int main()
{
    buffer = (float *)calloc(BUFFER_SIZE, sizeof(float)); // Zero-initialize buffer
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate buffer\n");
        return 1;
    }

    atexit(clean);

    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_OpenDefaultStream(&stream,
                               CHANNELS,  // Input channels
                               CHANNELS,  // Output channels
                               paFloat32, // 32-bit floating point
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               pa_callback,
                               NULL);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    printf("Press any key to stop...\n");
    getchar();
    // while(!feof(stdin))
    // {
    //     printf("read: %lu\nwrite: %lu\ndiff: %lu\n\n", r_index, w_index, w_index - r_index);
    // }

    err = Pa_StopStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    }

    Pa_Terminate();

    return 0;
}
