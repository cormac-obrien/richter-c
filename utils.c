#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

uint8_t *utils_read_binary_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Couldn't open %s.\n", path);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        perror(path);
        return NULL;
    }

    const long file_size = ftell(fp);
    if (file_size == -1) {
        perror(path);
        return NULL;
    }

    rewind(fp);

    uint8_t *data = calloc(file_size, sizeof *data);
    if (data == NULL) {
        perror("model_from_bsp");
        return NULL;
    }

    const size_t read_size = fread(data, sizeof *data, file_size, fp);
    if ((long)read_size != file_size) {
        fputs("Short read.\n", stderr);
        return NULL;
    }

    fclose(fp);
    fp = NULL;

    return data;
}

const struct utils_namespace Utils = {
    .readBinaryFile = utils_read_binary_file
};
