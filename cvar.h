#ifndef CVAR_H
#define CVAR_H

#include <stdbool.h>

extern const struct cvar_namespace {
    void (* const addString)(const char *name, const char *val, bool save);
    void (* const addNumber)(const char *name, float val, bool save);
    const char *(* const getString)(const char *name);
    float (*const getNumber)(const char *name);
} Cvar;

#endif
