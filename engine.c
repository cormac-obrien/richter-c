#include "engine.h"

static float engine_time_delta = 0.0f;
static uint32_t engine_frame_count = 0;

void engine_set_time_delta(float dt)
{
    engine_time_delta = dt;
}

float engine_get_time_delta()
{
    return engine_time_delta;
}

void engine_inc_frame_count()
{
    engine_frame_count++;
}

uint32_t engine_get_frame_count()
{
    return engine_frame_count;
}

const struct engine_namespace Engine = {
    .setTimeDelta = engine_set_time_delta,
    .getTimeDelta = engine_get_time_delta,
    .incFrameCount = engine_inc_frame_count,
    .getFrameCount = engine_get_frame_count
};
