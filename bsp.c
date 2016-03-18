#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp.h"
#include "utils.h"
#include "vecmath.h"

typedef struct {
    vec3_t normal;
} bsp_plane_t;

typedef struct {
    bool is_leaf;

    bsp_node_t *front;
    bsp_node_t *back;
} bsp_node_t;

typedef struct {
    bool is_leaf;
} bsp_leaf_t;

typedef struct {
    int node_count;
    bsp_node_t *nodes;

    int plane_count;
} bsp_t;

/*
 * Traverse the BSP tree until the leaf containing the specified point is
 * reached
 */
bsp_leaf_t *bsp_find_leaf_containing(bsp_t *bsp, vec3_t point)
{
    if (bsp == NULL) {
        return NULL;
    }

    bsp_node_t *node = bsp->nodes;
    while (!node->is_leaf) {
        if (vec3_dot(point, node->plane.normal)) {
            node = node->front;
        } else {
            node = node->back;
        }
    }

    return node;
}

bsp_t *bsp_load(const char *path)
{
    printf("Reading BSP data from %s...\n", path);

    uint8_t *bsp_data = Utils.readBinaryFile(path);
    if (bsp_data == NULL) {
        perror(path);
        return NULL;
    }

    bspfile_header_t *header = (bspfile_header_t *)bsp_data;

    if (header->version != BSP_VERSION) {
        fprintf(stderr, "BSP version: (got %d, expected %d)\n",
                header->version, BSP_VERSION);
        free(bsp_data);
        return NULL;
    }

    puts((char *)(bsp_data + header->lumps[LUMP_ENTITIES].offset));

    /*
     * Load vertices ===========================================================
     */

    /*
     * Sanity check for vertex count
     */
    div_t vertex_div = div(header->lumps[LUMP_VERTICES].size, sizeof (vec3_t));
    if (vertex_div.rem != 0) {
        fputs("BSP vertex data of invalid_size.\n", stderr);
        free(bsp_data);
        return NULL;
    }

    const int vertex_count = vertex_div.quot;
    vec3_t *vertices = calloc(vertex_count, sizeof *vertices);
    memcpy(vertices, bsp_data + header->lumps[LUMP_VERTICES].offset,
           vertex_count * sizeof *vertices);

    /*
     * Sanity check for plane count
     */
    div_t plane_div = div(header->lumps[LUMP_PLANES].size, sizeof (bspfile_plane_t));
    if (plane_div.rem != 0) {
        fputs("BSP plane data of invalid size.\n", stderr);
        free(bsp_data);
        return NULL;
    }

    const int plane_count = plane_div.quot;

    /*
     * Sanity check for node count
     */
    div_t node_div = div(header->lumps[LUMP_NODES].size, sizeof (bspfile_node_t));
    if (node_div.rem != 0) {
        fputs("BSP node data of invalid_size.\n", stderr);
        free(bsp_data);
        return NULL;
    }

    const int node_count = node_div.quot;
    return NULL;
}

const struct bsp_namespace BSP = {
    .load = bsp_load
};
