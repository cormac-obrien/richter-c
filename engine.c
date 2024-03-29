/*
 * Copyright © 2016 Cormac O'Brien
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"

static float engine_time_delta = 0.0f;
static uint32_t engine_frame_count = 0;

void engine_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void engine_fatal(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

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
    .error = engine_error,
    .fatal = engine_fatal,
    .setTimeDelta = engine_set_time_delta,
    .getTimeDelta = engine_get_time_delta,
    .incFrameCount = engine_inc_frame_count,
    .getFrameCount = engine_get_frame_count
};
