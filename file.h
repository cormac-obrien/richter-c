#ifndef FILE_H
#define FILE_H

extern const struct file_namespace {
    void (* const addDirToPath)(const char *path);
    void *(* const loadFromDisk)(const char *path);
} File;

#endif
