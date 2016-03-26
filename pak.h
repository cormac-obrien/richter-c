#ifndef PAK_H
#define PAK_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static const char * const PAK_MAGIC = "PACK";

typedef struct {
    /**
     * The magic number for the PAK archive format. Must be equivalent to
     * PAK_MAGIC.
     */
     char magic[4];

    /**
     * The offset in bytes from the beginning of the PAK data to the beginning
     * of the directory.
     */
    int32_t offset;

    /**
     * The size in bytes of the directory.
     */
    int32_t size;
} pak_header_t;

// static_assert(sizeof (pak_header_t) == 12, "Check PAK header size");

typedef struct {
    /**
     * A null-terminated character array representing the file name.
     */
    char path[56];

    /**
     * The offset in bytes from the beginngin of the PAK data to the beginning
     * of this file.
     */
    int32_t offset;

    /**
     * The size in bytes of this file.
     */
    int32_t size;
} pak_stat_t;

// static_assert(sizeof (pak_stat_t) == 64, "Check PAK file entry size");

typedef struct pak_s pak_t;
extern const struct pak_namespace {
    void (* const print)(const pak_t *pak);
    pak_t *(* const open)(const char *path);
    const void *(* const loadFile)(const pak_t *pak, const char *path);
} PAK;

#endif
