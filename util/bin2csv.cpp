#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *in = NULL, *out = NULL;
    char buf[512] = { 0 };
    int b;

    if (argc < 3) {
        printf("Syntax: %s <file> <output>\n", argv[0]);
        return 1;
    }

    in = fopen(argv[1], "rb");

    if (in == NULL) {
        printf("Invalid file %s\n", argv[1]);
        return 1;
    }

    out = fopen(argv[2], "w");

    if (out == NULL) {
        printf("Invalid output file %s\n", argv[2]);
        return 1;
    }

    for (b = fgetc(in); !feof(in);) {
        fprintf(out, "0x%x", b);
        b = fgetc(in);

        if (!feof(in)) {
            fprintf(out, ",");
        }
    }

    fclose(in);
    fclose(out);

    printf("Wrote %s.\n", argv[2]);
    return 0;
}