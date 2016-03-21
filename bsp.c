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
#include "utils.h"
#include "vecmath.h"

typedef struct {
    uint16_t endpoints[2];
} bsp_edge_t;

typedef struct {
    vec3_t normal;
} bsp_plane_t;

typedef struct {
    int id;
    int type;
} bsp_leaf_t;

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
    uint8_t *data;

    int vertex_count;
    vec3_t *vertices;

    int edge_count;
    bsp_edge_t *edges;

    int edgetable_count;
    int *edgetable;

    int plane_count;
    bsp_plane_t *planes;

    int leaf_count;
    bsp_leaf_t *leaves;

    int node_count;
    bsp_node_t *nodes;

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
    while (node->plane_index > 0) {
        if (vec3_dot(point, node->plane->normal) >= 0) {
            printf("front: %p\n", node->front);
            if (node->front == node) {
                puts("node->front points to itself");
            }
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

void bsp_load_textures(bsp_t *bsp, bspfile_texheader_t *data, int size)
{
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

bsp_t *bsp_load(const char *path)
{
    void *bsp_data = Utils.readBinaryFile(path);
    void *elements[LUMP_COUNT];
    int   sizes[LUMP_COUNT];

    bspfile_header_t *header = (bspfile_header_t *)bsp_data;
    for (int i = 0; i < LUMP_COUNT; i++) {
        elements[i] = (uint8_t *)bsp_data + header->lumps[i].offset;
        sizes[i] = header->lumps[i].size;
    }

    bsp_t *bsp = calloc(1, sizeof *bsp);

    bsp_load_vertices(bsp, elements[LUMP_VERTICES], sizes[LUMP_VERTICES]);
    bsp_load_edges(bsp, elements[LUMP_EDGES], sizes[LUMP_EDGES]);
    bsp_load_edgetable(bsp, elements[LUMP_EDGETABLE], sizes[LUMP_EDGETABLE]);
    bsp_load_leaves(bsp, elements[LUMP_LEAVES], sizes[LUMP_LEAVES]);
    bsp_load_nodes(bsp, elements[LUMP_NODES], sizes[LUMP_NODES]);

    vec3_t zero = {0.f, 0.f, 0.f};
    bsp_leaf_t *leaf = bsp_find_leaf_containing(bsp, zero);
    printf("Point is in leaf with id %d\n", leaf->id);

    return bsp;
}

const struct bsp_namespace BSP = {
    .load = bsp_load
};
