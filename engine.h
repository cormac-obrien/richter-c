#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

extern const struct engine_namespace {
    void (* const setTimeDelta)(float dt);
    float (* const getTimeDelta)();
    void (* const incFrameCount)();
    uint32_t (* const getFrameCount)();
} Engine;

#endif
