#ifndef FFT_H
#define FFT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define PI 3.14159265358979323846

// Function to perform the FFT
void fft(float *real, float *imag, int n)
{
    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n; i++)
    {
        if (i < j)
        {
            // Swap real and imaginary parts
            double tempReal = real[i];
            double tempImag = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tempReal;
            imag[j] = tempImag;
        }
        int m = n / 2;
        while (m >= 1 && j >= m)
        {
            j -= m;
            m /= 2;
        }
        j += m;
    }

    // Cooley-Tukey FFT
    for (int step = 2; step <= n; step *= 2)
    {
        float angle = -2.0 * PI / step;
        float wReal = cos(angle);
        float wImag = sin(angle);
        for (int k = 0; k < n; k += step)
        {
            float uReal = 1.0;
            float uImag = 0.0;
            for (int m = 0; m < step / 2; m++)
            {
                int i1 = k + m;
                int i2 = k + m + step / 2;

                float tReal = uReal * real[i2] - uImag * imag[i2];
                float tImag = uReal * imag[i2] + uImag * real[i2];

                real[i2] = real[i1] - tReal;
                imag[i2] = imag[i1] - tImag;

                real[i1] += tReal;
                imag[i1] += tImag;

                float tempReal = uReal * wReal - uImag * wImag;
                uImag = uReal * wImag + uImag * wReal;
                uReal = tempReal;
            }
        }
    }
}

bool is_frequency_present(double *samples, int sample_rate, int num_samples, double target_frequency, double threshold)
{
    // Create real and imaginary arrays for FFT
    double *real = (double *)malloc(num_samples * sizeof(double));
    double *imag = (double *)malloc(num_samples * sizeof(double));
    if (!real || !imag)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    // Copy samples into the real array; imaginary part is initialized to 0
    for (int i = 0; i < num_samples; i++)
    {
        real[i] = samples[i];
        imag[i] = 0.0;
    }

    // Perform FFT
    fft(real, imag, num_samples);

    // Calculate the magnitude for each frequency bin
    double bin_width = (double)sample_rate / num_samples; // Frequency range per FFT bin
    for (int i = 0; i < num_samples / 2; i++)
    {
        double frequency = i * bin_width;
        double magnitude = sqrt(real[i] * real[i] + imag[i] * imag[i]);

        // Check if the target frequency is present above the threshold
        if (fabs(frequency - target_frequency) < bin_width / 2 && magnitude > threshold)
        {
            free(real);
            free(imag);
            return true;
        }
    }
}
#endif