#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "file.h"
#include "pak.h"

#define FILE_MAX_PATH_LEN (128)

typedef struct paklist_s {
    const pak_t *pak;
    struct paklist_s *next;
} paklist_t;

static paklist_t *pak_list= NULL;

void file_list_path()
{
    if (pak_list == NULL) {
        Engine.error("No files in path.\n");
    }
    for (paklist_t *node = pak_list; node != NULL; node = node->next) {
        PAK.print(node->pak);
    }
}

/**
 * Adds the PAK archive pointed to by \p pak to the engine's search path.
 * @parak pak The PAK archive to be added
 */
void file_add_pak_to_path(const pak_t *pak)
{
    paklist_t *node = calloc(1, sizeof *node);
    node->pak = pak;
    node->next = pak_list;
    pak_list = node;
}

/**
 * Adds the directory specified by \p path to the engine's search path.
 * @param path The path to the directory to be added
 */
void file_add_dir_to_path(const char *path)
{
    /* Search path for PAK archives and add them to the search path */
    static char pak_path[FILE_MAX_PATH_LEN];
    for (int paknum = 0; ; paknum++) {
        /* TODO: might want to check both upper- and lowercase */
        snprintf(pak_path, FILE_MAX_PATH_LEN, "%s/PAK%d.PAK", path, paknum);
        pak_t *pak = PAK.open(pak_path);
        if (pak == NULL) {
            break;
        }

        file_add_pak_to_path(pak);
    }
    file_list_path();
}

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
    .addDirToPath = file_add_dir_to_path,
    .loadFromDisk = file_load_from_disk
};
