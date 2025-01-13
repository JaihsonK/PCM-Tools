#include <stdlib.h>
#include <math.h>

typedef struct
{
    unsigned frequencies_found;
    unsigned samples;
    struct
    {
        unsigned frequency;
        float *pcm;
    } *bins;
}analyses;

float *x_real, *x_imag;

analyses find_freq(float *pcm, unsigned samples)
{
    x_imag = (float *)malloc(sizeof(float) * samples);
    x_real = (float *)malloc(sizeof(float) * samples);


}