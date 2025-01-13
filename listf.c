#include "fft.h"
#include <stdio.h>

float *pcm;
int channels, sample_rate, total_samples;

struct window
{
    float *start;
    int width;
    int windows_progressed;
} window;

unsigned long long flags[55000 / 64];

bool is_set(float freq)
{
    int f = (int)freq;
    int index = ((freq * 10) - ((int)freq * 10) / 2);

    
}

int main()
{
    fread(&sample_rate, sizeof(sample_rate), 1, stdin);
    fread(&channels, sizeof(channels), 1, stdin);
    fread(&total_samples, sizeof(total_samples), 1, stdin);

    if(channels != 1)
        return -1;

    pcm = (float *)malloc(sizeof(float) * total_samples);
    fread(pcm, total_samples * sizeof(float), 1, stdin);

    window.width = 22000; // highest frequency produced by human voice is 11khz
    window.start = pcm;

    while(window.windows_progressed <= total_samples / window.width)
    {
        for(float freq = window.width / 2.0f; freq > 0.0f; freq -= 0.2f)
        {
            if(is_frequency_present(window.start, sample_rate, window.width, freq, 0.5))
            {

            }
        }
    }
}