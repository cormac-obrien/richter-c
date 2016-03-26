#ifndef FILE_H
#define FILE_H

extern const struct file_namespace {
    void *(* const loadFromDisk)(const char *path);
} File;

#endif
