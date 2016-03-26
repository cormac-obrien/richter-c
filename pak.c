/*
 * Copyright © 2016 Cormac O'Brien.
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

/** @file pak.c */

#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "file.h"
#include "pak.h"

#define PAK_MAX_PATH_LENGTH (56)

/** Handle to a file in a PAK archive. */
typedef struct {
    /** The path to this file in the PAK archive. */
    const char *path;

    /** The size in bytes of this file. */
    size_t size;

    /** A pointer to the data in this file. */
    const void *data;
} pak_file_t;

/** Internal representation of a PAK archive. */
typedef struct pak_s {
    /** The number of files contained in this PAK archive. */
    size_t      file_count;

    /** An array of PAK file handles */
    pak_file_t *files;
} pak_t;

void pak_print(const pak_t *pak)
{
    for (size_t i = 0; i < pak->file_count; i++) {
        puts(pak->files[i].path);
    }
}

/**
 * Loads a PAK archive from \p path and returns a handle to it.
 * @param path The path to the PAK file to be loaded
 * @return A handle to the PAK file specified by \p path
 */
pak_t *pak_open(const char *path)
{
    void *pak_data = File.loadFromDisk(path);
    if (pak_data == NULL) {
        return NULL;
    }

    /* Check magic number */
    pak_header_t *header = pak_data;
    for (size_t i = 0; i < 4; i++) {
        if (header->magic[i] != PAK_MAGIC[i]) {
            Engine.error("PAK archive '%s' has bad magic number\n", path);
            return NULL;
        }
    }

    /* Check archive size parity and calculate file count */
    const pak_stat_t *directory = pak_data + header->offset;
    if (header->size % sizeof *directory != 0) {
        Engine.error("PAK archive '%s' directory has bad size\n", path);
        return NULL;
    }
    size_t file_count = header->size / sizeof *directory;

    pak_file_t *files = calloc(file_count, sizeof *files);
    if (files == NULL) {
        Engine.error("Failed to allocate memory for files in PAK archive '%s'\n", path);
        return NULL;
    }

    for (size_t i = 0; i < file_count; i++) {
        files[i].path = directory[i].path;
        files[i].size = directory[i].size;
        files[i].data = (uint8_t *)pak_data + directory[i].offset;
    }

    pak_t *pak = calloc(1, sizeof *pak);
    pak->file_count = file_count;
    pak->files = files;

    return pak;
}

const void *pak_load_file(const pak_t *pak, const char *path)
{
    if (pak == NULL) {
        Engine.error("PAK was null.\n");
        return NULL;
    }

    if (path == NULL) {
        Engine.error("Path was null.\n");
        return NULL;
    }

    for (size_t i = 0; i < pak->file_count; i++) {
        if (strncmp(pak->files[i].path, path, PAK_MAX_PATH_LENGTH)) {
            return pak->files[i].data;
        }
    }

    Engine.error("'%s' not found in the given PAK archive.\n", path);
    return NULL;
}

const struct pak_namespace PAK = {
    .print = pak_print,
    .open = pak_open,
    .loadFile = pak_load_file
};
