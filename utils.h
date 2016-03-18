#ifndef UTILS_H
#define UTILS_H

extern const struct utils_namespace {
    uint8_t *(* const readBinaryFile)(const char *path);
} Utils;

#endif
