/**  PCM rip
 * built with
 * cc pcmrip.c -lm-o pcmrip
 */

#include <stdio.h>
#include <stdlib.h>
#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"
#include "minimp3/minimp3_ex.h"
#include <math.h>
#include <stdbool.h>


int main(int argc, char **argv)
{
    if (argc < 2 || !argv[1])
    {
        fprintf(stderr, "Usage: %s <mp3_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        perror("Error opening MP3 file");
        return 1;
    }

    // Read the entire file into memory
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *mp3_data = (unsigned char *)malloc(file_size);
    if (!mp3_data)
    {
        fprintf(stderr, "Failed to allocate memory for MP3 data\n");
        fclose(file);
        return 1;
    }
    fread(mp3_data, 1, file_size, file);
    fclose(file);

    // Initialize the MP3 decoder
    mp3dec_ex_t mp3d;
    if (mp3dec_ex_open_buf(&mp3d, mp3_data, file_size, MP3D_SEEK_TO_SAMPLE) != 0)
    {
        fprintf(stderr, "Failed to initialize MP3 decoder\n");
        free(mp3_data);
        return 1;
    }

    // Decode the MP3 file into raw PCM samples
    int samples_decoded = mp3d.samples;
    int sample_rate = mp3d.info.hz;
    int channels = mp3d.info.channels;
    int total_samples = samples_decoded * channels;

    fprintf(stderr, "Decoded MP3 Info:\n");
    fprintf(stderr, "Sample Rate: %d Hz\n", sample_rate);
    fprintf(stderr, "Channels: %d\n", channels);
    fprintf(stderr, "Total Samples: %d\n", total_samples);

    float *pcm_samples = (float *)malloc(total_samples * sizeof(float));
    short *tmp = (short *)malloc(total_samples * sizeof(short));
    if (!pcm_samples || !tmp)
    {
        fprintf(stderr, "Failed to allocate memory for PCM samples\n");
        mp3dec_ex_close(&mp3d);
        free(mp3_data);
        if (tmp)
            free(tmp);
        else
            free(pcm_samples);
        return 1;
    }

    int samples_read = mp3dec_ex_read(&mp3d, tmp, total_samples);
    if (samples_read <= 0)
    {
        fprintf(stderr, "Error decoding MP3 data\n");
        free(pcm_samples);
        mp3dec_ex_close(&mp3d);
        free(mp3_data);
        return 1;
    }

    // convert int pcm samples to float pcm samples
    for (unsigned i = 0; i < total_samples; i++)
    {
        pcm_samples[i] = ((float)tmp[i]) / ((float)INT16_MAX + 1.0);
        //fprintf(stderr, "%i %f\n", tmp[i], pcm_samples[i]); //logging
    }

    fwrite(&sample_rate, sizeof(sample_rate), 1, stdout);
    fwrite(&channels, sizeof(channels), 1, stdout);
    fwrite(&total_samples, sizeof(total_samples), 1, stdout);

    fwrite(pcm_samples, sizeof(float), total_samples, stdout);

    fprintf(stderr, "Successfully decoded %d samples.\n", samples_read);

    mp3dec_ex_close(&mp3d);
    free(mp3_data);
    free(tmp);
    free(pcm_samples);

    return 0;
}
