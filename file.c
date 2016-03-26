#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "file.h"

/**
 * Loads the file with path \p path from the file system and returns a
 * null-terminated buffer containing its data.
 * @param path The path to the file in the file system.
 * @return The data contained in the file at \p path, or NULL on error.
 *
 * TODO: rename to distinguish from generic loading functions that load from
 * both FS and PAK archives
 */
void *file_load_from_disk(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        return NULL;
    }

    const long file_size = ftell(fp);
    if (file_size == -1) {
        return NULL;
    }

    rewind(fp);

    uint8_t *data = calloc(file_size, sizeof *data + 1);
    if (data == NULL) {
        return NULL;
    }

    const size_t read_size = fread(data, sizeof *data, file_size, fp);
    if ((long)read_size != file_size) {
        return NULL;
    }

    fclose(fp);
    fp = NULL;

    return (void *)data;
}

const struct file_namespace File = {
    .loadFromDisk = file_load_from_disk
};
