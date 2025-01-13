#include <stdio.h>
#include <stdlib.h>

int main()
{
    int tmp;
    int samples;
    fread(&tmp, sizeof(tmp), 1, stdin);
    fwrite(&tmp, sizeof(tmp), 1, stdout);
    fread(&samples, sizeof(samples), 1, stdin);
    fwrite(&samples, sizeof(samples), 1, stdout);
    fread(&tmp, sizeof(tmp), 1, stdin);
    fwrite(&tmp, sizeof(tmp), 1, stdout);

    float pcm;
    for (unsigned i = 0; i < samples && !feof(stdin); i++)
    {
        fread(&pcm, sizeof(float), 1, stdin);
        pcm = -pcm;
        fwrite(&pcm, sizeof(float), 1, stdout);
    }
    return 0;
}