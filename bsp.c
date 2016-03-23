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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp.h"
#include "engine.h"
#include "utils.h"
#include "vecmath.h"

typedef struct {
    uint16_t endpoints[2];
} bsp_edge_t;

typedef struct {
    int id;
    int type;
} bsp_leaf_t;

typedef struct {
    vec3_t   normal;
    float    offset;
    uint32_t type;
} bsp_plane_t;

/*
 * Internal representation of a node in a BSP tree
 */
typedef struct bsp_node_s {
    /*
     * This field is the same as the bspfile_node_t's plane_index field, kept
     * here solely to determine if this is a node or a leaf.
     */
    int id;
    int plane_index;

    /*
     * The frame when this node was traversed last. If this equals the current
     * frame count, this node's children need to be examined.
     */
    int last_visited;

    /*
     * A direct pointer to this node's plane is stored to avoid having to index
     * into the BSP's plane array for every node every frame.
     */
    bsp_plane_t *plane;

    struct bsp_node_s *front;
    struct bsp_node_s *back;
} bsp_node_t;


typedef struct bsp_s {
    int vertex_count;
    vec3_t *vertices;

    int edge_count;
    bsp_edge_t *edges;

    int edgetable_count;
    int *edgetable;

    int texture_count;
    bsp_texture_t **textures;

    uint8_t *vislists;

    int leaf_count;
    bsp_leaf_t *leaves;

    int plane_count;
    bsp_plane_t *planes;

    int node_count;
    bsp_node_t *nodes;

    int model_count;
    bsp_model_t *models;

} bsp_t;

/*
 * Returns a pointer to the leaf that contains the specified point.
 */
bsp_leaf_t *bsp_find_leaf_containing(bsp_t *bsp, vec3_t point)
{
    if (bsp == NULL) {
        return NULL;
    }

    bsp_node_t *node = bsp->nodes;
    while (node->plane_index >= 0) {
        if (vec3_dot(point, node->plane->normal) >= 0) {
            if (node->front == node) {
                puts("node->front points to itself");
            }
            puts("front");
            node = node->front;
        } else {
            if (node->back == node) {
                puts("node->back points to itself");
            }
            puts("back");
            node = node->back;
        }
    }

    return (bsp_leaf_t *)node;
}

void bsp_load_vertices(bsp_t *bsp, vec3_t *data, int size)
{
    if (size % sizeof *data != 0) {
        fputs("Vertex data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    vec3_t *vertices = calloc(count, sizeof *vertices);

    for (int i = 0; i < count; i++) {
        vec3_copy(vertices[i], data[i]);
    }

    bsp->vertex_count = count;
    bsp->vertices = vertices;
}

/**
 * Loads \p size bytes' worth of edges from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the edges
 * @param data An array of edges to be loaded
 * @param size The size in bytes of \p data
 */
void bsp_load_edges(bsp_t *bsp, bspfile_edge_t *data, int size)
{
    if (size % sizeof *data != 0) {
        fputs("Edge data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    bsp_edge_t *edges = calloc(count, sizeof *edges);

    for (int i = 0; i < count; i++) {
        edges[i].endpoints[0] = data[i].endpoints[0];
        edges[i].endpoints[1] = data[i].endpoints[1];
    }

    bsp->edge_count = count;
    bsp->edges = edges;
}

/**
 * Loads \p size bytes' worth of edge indices from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the edge indices
 * @param data An array of edge indices to be loaded
 * @param size The size in bytes of \p data
 */
void bsp_load_edgetable(bsp_t *bsp, int *data, int size)
{
    if (size % sizeof *data != 0) {
        fputs("Edge list data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    int *edgetable = calloc(count, sizeof *edgetable);

    for (int i = 0; i < count; i++) {
        edgetable[i] = data[i];
    }

    bsp->edgetable = edgetable;
}

/**
 * Loads \p size bytes' worth of texture data from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the texture data
 * @param data A pointer to a texture header listing the textures
 * @param size The total size of all texture data
 *
 * TODO: It probably makes sense to send the textures to the GPU here so the
 * data doesn't need to be stored
 */
void bsp_load_textures(bsp_t *bsp, bspfile_texheader_t *data, int size)
{
    bsp_texture_t **textures = calloc(data->texture_count, sizeof *textures);

    for (int i = 0; i < data->texture_count; i++) {
        bspfile_texture_t *texdata =
                (bspfile_texture_t *)((uint8_t *)data + data->offsets[i]);

        if (texdata->width % 16 != 0 || texdata->height % 16 != 0) {
            Engine.fatal("Texture '%s' has illegal dimensions.\n", texdata->name);
        }

        /*
         * Ratio of mipmap pixels to texture pixels:
         * (8x8 + 4x4 + 2x2 + 1x1) / (8x8) = 85/64
         */
        int pixel_count = texdata->width * texdata->height * (85/64);

        textures[i] = calloc(1, sizeof **textures + pixel_count);
        strncpy(textures[i]->name, texdata->name, 15);
        textures[i]->width = texdata->width;
        textures[i]->height = texdata->height;
        textures[i]->offset_full = texdata->offset_full;
        textures[i]->offset_half = texdata->offset_half;
        textures[i]->offset_quarter = texdata->offset_quarter;
        textures[i]->offset_eighth = texdata->offset_eighth;

        memcpy(&textures[i] + sizeof **textures, texdata + sizeof texdata, pixel_count);
    }
}

/**
 * Loads \p size bytes of visibility data from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the visibility data
 * @param data An array of bytes containing visibility data
 * @param size The size in bytes of \p data
 */
void bsp_load_vislists(bsp_t *bsp, uint8_t  *data, int size)
{
    uint8_t *vislists = calloc(size, sizeof *data);
    if (vislists == NULL) {
        Engine.fatal("Vislist allocation failed.\n");
    }
    memcpy(vislists, data, size);
    bsp->vislists = vislists;
}

/**
 * Loads \p size bytes' worth of leaves from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the leaf data
 * @param data An array of leaves to be loaded
 * @param size The size in bytes of \p data
 */
void bsp_load_leaves(bsp_t *bsp, bspfile_leaf_t *data, int size)
{
    if (size % sizeof *data != 0) {
        fputs("Leaf data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    bsp_leaf_t *leaves = calloc(count, sizeof *leaves);

    for (int i = 0; i < count; i++) {
        leaves[i].id = i;
        leaves[i].type = data[i].type;
    }

    bsp->leaves = leaves;
}

void bsp_load_planes(bsp_t *bsp, bspfile_plane_t *data, int size)
{
    if (size % sizeof *data != 0) {
        fputs("Plane data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    bsp_plane_t *planes = calloc(count, sizeof *planes);

    for (int i = 0; i < count; i++) {
        vec3_copy(planes[i].normal, data[i].normal);
        planes[i].offset = data[i].offset;
        planes[i].type = data[i].type;
    }

    bsp->planes = planes;
}

/**
 * Loads \p size bytes' worth of nodes from \p data into \p bsp.
 * @param bsp A pointer to the BSP struct in which to store the node data
 * @param data An array of nodes to be loaded
 * @param size The size in bytes of \p data
 *
 * TODO: factor out cycle check
 */
void bsp_load_nodes(bsp_t *bsp, bspfile_node_t *data, int size)
{
    /*
     * Make sure size is of correct parity
     */
    if (size % sizeof *data != 0) {
        fputs("Node data has bad size.\n", stderr);
        return;
    }

    int count = size / sizeof *data;
    bsp_node_t *nodes = calloc(count, sizeof *nodes);

    /*
     * Set up table for cycle checking
     */
    int visited[count];
    for (int i = 0; i < count; i++) {
        visited[i] = false;
    }

    for (int i = 0; i < count; i++) {
        nodes[i].id = i;
        nodes[i].plane_index = data[i].plane_index;
        nodes[i].plane = &bsp->planes[i];
        printf("Node %d:\n", i);

        const int front = data[i].front;
        if (front < 0) {
            nodes[i].front = (bsp_node_t *)&bsp->leaves[~front];
            printf("  Front = Leaf %d\n", ~front);
        } else {
            if (visited[front]) {
                fprintf(stderr, "Cycle detected: Node %d already has parent\n", front);
            }
            visited[front] = true;
            nodes[i].front = &nodes[front];
            printf("  Front = Node %d\n", front);
        }

        const int back = data[i].back;
        if (back < 0) {
            nodes[i].back = (bsp_node_t *)&bsp->leaves[~back];
            printf("  Back  = Leaf %d\n", ~back);
        } else {
            if (visited[back]) {
                fprintf(stderr, "Cycle detected: Node %d already has parent\n", back);
            }
            visited[back] = true;
            nodes[i].back = &nodes[back];
            printf("  Back  = Node %d\n", back);
        }
    }

    bsp->nodes = nodes;
}

void bsp_load_models(bsp_t *bsp, bspfile_model_t *data, int size)
{
    if (size % sizeof *data != 0) {
        Engine.fatal("Model data has bad size.\n");
    }

    int count = size / sizeof *data;
    bsp_model_t *models = calloc(count, sizeof *models);

    for (int i = 0; i < count; i++) {
        models[i] = data[i];
    }

    bsp->models = models;
}

/**
 * Loads a BSP tree from the map file indicated by \p path.
 * @param path The path of the BSP file to be loaded
 * @return A fully populated BSP tree representing the map
 */
bsp_t *bsp_load(const char *path)
{
    void *bsp_data = Utils.readBinaryFile(path);

    /*
     * Calculate pointers to and sizes of each lump
     */
    void *elements[LUMP_COUNT];
    int   sizes[LUMP_COUNT];
    bspfile_header_t *header = (bspfile_header_t *)bsp_data;
    for (int i = 0; i < LUMP_COUNT; i++) {
        elements[i] = (uint8_t *)bsp_data + header->lumps[i].offset;
        sizes[i] = header->lumps[i].size;
    }

    bsp_t *bsp = calloc(1, sizeof *bsp);

    /*
     * The order is arbitrary since the BSP tree is not actually read until it
     * is fully loaded, but this order ensures that there are no dangling
     * pointers just in case.
     */
    bsp_load_vertices(bsp, elements[LUMP_VERTICES], sizes[LUMP_VERTICES]);
    bsp_load_edges(bsp, elements[LUMP_EDGES], sizes[LUMP_EDGES]);
    bsp_load_edgetable(bsp, elements[LUMP_EDGETABLE], sizes[LUMP_EDGETABLE]);
    bsp_load_vislists(bsp, elements[LUMP_VISLISTS], sizes[LUMP_VISLISTS]);
    bsp_load_textures(bsp, elements[LUMP_TEXTURES], sizes[LUMP_TEXTURES]);
    bsp_load_leaves(bsp, elements[LUMP_LEAVES], sizes[LUMP_LEAVES]);
    bsp_load_planes(bsp, elements[LUMP_PLANES], sizes[LUMP_PLANES]);
    bsp_load_nodes(bsp, elements[LUMP_NODES], sizes[LUMP_NODES]);
    bsp_load_models(bsp, elements[LUMP_MODELS], sizes[LUMP_MODELS]);

    return bsp;
}

const struct bsp_namespace BSP = {
    .load = bsp_load
};
