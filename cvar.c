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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cvar.h"

typedef enum {
    NUMBER,
    STRING
} cvar_valtype_t;

typedef struct cvar_s {
    struct cvar_s *next;
    char          *name;
    union {
        char *string;
        float number;
    } value;

    cvar_valtype_t value_type;
    bool           save;
    bool           server;
} cvar_t;

cvar_t *cvars;

const cvar_t *cvar_find(const char *name)
{
    for (cvar_t *var = cvars; var != NULL; var = var->next) {
        if (strcmp(name, var->name) == 0) {
            printf("Found %s.\n", name);
            return var;
        }
    }

    return NULL;
}

bool cvar_exists(const char *name)
{
    return cvar_find(name) != NULL;
}

void cvar_add_string(const char *name, const char *val, bool save)
{
    /*
     * Check for name collision with other cvars
     */
    if (cvar_exists(name)) {
        printf("Cvar %s already exists.\n", name);
        return;
    }

    /*
     * TODO: check for name collision with commands
     */

    /*
     * Copy cvar name
     */
    cvar_t *new_cvar = calloc(1, sizeof *new_cvar);
    new_cvar->name = calloc(strlen(name), sizeof *new_cvar->name);
    strcpy(new_cvar->name, name);

    /*
     * Copy value
     */
    new_cvar->value_type = STRING;
    new_cvar->value.string = calloc(strlen(val), sizeof *new_cvar->value.string);
    strcpy(new_cvar->value.string, val);

    new_cvar->save = save;

    /*
     * Prepend new cvar to cvar list
     */
    new_cvar->next = cvars;
    cvars = new_cvar;
}

void cvar_add_number(const char *name, float val, bool save)
{
    /*
     * Check for name collision with other cvars
     */
    if (cvar_exists(name)) {
        printf("Cvar %s already exists.\n", name);
        return;
    }

    /*
     * TODO: check for name collision with commands
     */

    /*
     * Copy cvar name
     */
    cvar_t *new_cvar = calloc(1, sizeof *new_cvar);
    new_cvar->name = calloc(strlen(name), sizeof *new_cvar->name);
    strcpy(new_cvar->name, name);

    /*
     * Copy value
     */
    new_cvar->value_type = NUMBER;
    new_cvar->value.number = val;

    new_cvar->save = save;

    /*
     * Prepend new cvar to cvar list
     */
    new_cvar->next = cvars;
    cvars = new_cvar;
}

const char *cvar_get_string(const char *name)
{
    const cvar_t *var = cvar_find(name);

    if (var->value_type == STRING) {
        return var->value.string;
    }

    return NULL;
}

float cvar_get_number(const char *name)
{
    const cvar_t *var = cvar_find(name);

    if (var->value_type == NUMBER) {
        return var->value.number;
    }

    return 0.0f;
}

const struct cvar_namespace Cvar = {
    .addNumber = cvar_add_number,
    .addString = cvar_add_string,
    .getNumber = cvar_get_number,
    .getString = cvar_get_string
};
