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

#ifndef BSP_H
#define BSP_H

#include <stdint.h>

#define BSP_VERSION (29)

typedef float vec3_t[3];

enum {
    LUMP_ENTITIES = 0,
    LUMP_PLANES = 1,
    LUMP_TEXTURES = 2,
    LUMP_VERTICES = 3,
    LUMP_VISLISTS = 4,
    LUMP_NODES = 5,
    LUMP_TEXINFO = 6,
    LUMP_FACES = 7,
    LUMP_LIGHTMAPS = 8,
    LUMP_CLIPNODES = 9,
    LUMP_LEAVES = 10,
    LUMP_FACETABLE = 11,
    LUMP_EDGES = 12,
    LUMP_EDGETABLE = 13,
    LUMP_MODELS = 14,
    LUMP_COUNT = 15
};

typedef struct {
    int32_t offset;
    int32_t size;
} bspfile_lump_t;

typedef struct {
    int32_t version;
    bspfile_lump_t lumps[LUMP_COUNT];
} bspfile_header_t;

typedef struct {
    vec3_t min;
    vec3_t max;
} bspfile_bounds_t;

typedef struct {
    int16_t min[3];
    int16_t max[3];
} bspfile_shortbounds_t;

typedef struct {
    bspfile_bounds_t bounds;
    vec3_t origin;

    /*
     * Index of the first BSP node
     */
    int32_t bsp_index;

    /*
     * Indices of the bounding clip nodes
     */
    int32_t clip_index[2];

    int32_t zero;

    /*
     * Total number of leaves in the BSP tree associated with this model
     */
    int32_t leaf_count;
    int32_t face_index;
    int32_t face_count;
} bspfile_model_t;
typedef bspfile_model_t bsp_model_t;

typedef struct {
    uint16_t endpoints[2];
} bspfile_edge_t;

typedef struct {
    vec3_t vector_u;
    float  offset_u;
    vec3_t vector_v;
    float  offset_v;
    uint32_t texture_index;
    uint32_t is_animated;
} bspfile_texinfo_t;

typedef struct {
    uint16_t plane_index;
    uint16_t is_backface;
    int32_t  edge_index;
    int32_t  edge_count;
    uint16_t texture_info_index;
    uint8_t  light_type;
    uint8_t  light_min;
    uint8_t  light[2];
    int32_t  lightmap;
} bspfile_face_t;

typedef struct {
    int32_t texture_count;

    /*
     * Flexible array of texture_count offsets
     */
    int32_t offsets[];
} bspfile_texheader_t;

typedef struct {
    char name[16];
    uint32_t width;
    uint32_t height;

    /*
     * Offsets into the color index array for each mipmap.
     */
    uint32_t offset_full;
    uint32_t offset_half;
    uint32_t offset_quarter;
    uint32_t offset_eighth;
} bspfile_texture_t;
typedef bspfile_texture_t bsp_texture_t;

typedef struct {
    /*
     * Index of the plane that partitions this node. Note that a negative value
     * here indicates that the node is actually a leaf!
     */
    int32_t plane_index;

    /*
     * If the MSB in front/back is not set, then front/back is the index of the
     * front/back child node. If it is set, then the bitwise negation of
     * front/back is the index of the front/back child leaf.
     */
    int16_t front;
    int16_t back;

    bspfile_shortbounds_t bounds;
    uint16_t face_index;
    uint16_t face_count;
} bspfile_node_t;

typedef struct {
    int32_t type;

    /*
     * Index of the visibility list for this leaf. If this is -1, this leaf has
     * no visibility list and can see every other leaf.
     */
    int32_t visibility_list;
    bspfile_shortbounds_t bounds;

    /*
     * Index of the face list for this leaf.
     */
    uint16_t face_list;

    /*
     * Total number of faces in this leaf.
     */
    uint16_t face_count;
    uint8_t sound_water;
    uint8_t sound_sky;
    uint8_t sound_acid;
    uint8_t sound_lava;
} bspfile_leaf_t;

/*
 * Partitioning plane stored in point-normal form
 */
typedef struct {
    vec3_t normal;
    float  offset;
    int32_t type;
} bspfile_plane_t;

typedef struct {
    uint32_t plane_index;
    int16_t front;
    int16_t back;
} bspfile_clipnode_t;

typedef struct bsp_s bsp_t;

extern const struct bsp_namespace {
    bsp_t *(* const load)(const char *path);
} BSP;

#endif
