/*
 * Remove background sound from an audio sample of a voice
 * input: pcm stream (from pcmrip) over stdin
 */

#include <stdio.h>
#include <stdlib.h>

#include "fft.h"

float *pcm, *noiseless_pcm;
int channels, sample_rate, total_samples;

struct window
{
    float *start;
    int width;
    int windows_progressed;
} window;

int main()
{
    fread(&sample_rate, sizeof(sample_rate), 1, stdin);
    fread(&channels, sizeof(channels), 1, stdin);
    fread(&total_samples, sizeof(total_samples), 1, stdin);

    if(channels != 1)
        return -1;

    pcm = (float *)malloc(sizeof(float) * total_samples);
    fread(pcm, total_samples * sizeof(float), 1, stdin);

    noiseless_pcm = (float *)malloc(sizeof(float) * total_samples);

    // remove sound

    window.start = pcm;
    window.width = (sample_rate / 25) * channels; // 40 miliseconds
    fprintf(stderr, "Processing\n");
    while (window.start + window.width < pcm + total_samples)
    {
        for (float i = sample_rate / 2.0f; i > 0.0; i -= 0.2)
        {
            if (is_frequency_present(window.start, sample_rate, window.width, i, 0.2, false))
            {
                for(unsigned j = 0; j < window.width; j++)
                    noiseless_pcm[j + window.windows_progressed] = sinf(2.0f * PI * i * ((j + window.windows_progressed) / sample_rate));
            }
        }
        window.start += channels;
        window.windows_progressed += channels;
    }

    fwrite(&sample_rate, sizeof(sample_rate), 1, stdout);
    fwrite(&channels, sizeof(channels), 1, stdout);
    fwrite(&total_samples, sizeof(total_samples), 1, stdout);
    fwrite(noiseless_pcm, sizeof(float) * total_samples, 1, stdout);

    is_frequency_present(NULL, 0, 0, 0.0, 0.0, true);
    free(pcm);
    free(noiseless_pcm);

    return 0;
}