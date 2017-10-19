#include <stdio.h>
#include <stdlib.h>

/**
 * a utility to convert a file to a CSV of bytes
 */
int main(int argc, char *argv[])
{
    FILE *in = stdin, *out = stdout;
    int b;

    for (b = fgetc(in); !feof(in);) {
        fprintf(out, "0x%x", b);
        b = fgetc(in);

        if (!feof(in)) {
            fprintf(out, ",");
        }
    }
    return 0;
}