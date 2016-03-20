/*
 * Copyright © 2016 Cormac O'Brien
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

void *utils_read_binary_file(const char *path)
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

    return (void *)data;
}

const struct utils_namespace Utils = {
    .readBinaryFile = utils_read_binary_file
};
