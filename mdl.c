#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include "vecmath.h"

#include "mdl.h"
#include "utils.h"

#define MDL_MAGIC (0x4F504449)
#define MDL_VERSION (6)

typedef float vec3_t[3];

typedef struct mdl_header
{
    int32_t magic;      /* MDL format magic number, must be MDL_MAGIC */
    int32_t version;    /* MDL version, must be MDL_VERSION */
    vec3_t  scale;      /* x-, y-, and z-scaling factors */
    vec3_t  origin;
    float   radius;     /* bounding radius */
    vec3_t  eyes;
    int32_t skin_count; /* number of textures used by this model */
    int32_t skin_w;
    int32_t skin_h;
    int32_t texcoord_count;
    int32_t triangle_count;
    int32_t frame_count; /* number of animation frames */
    int32_t sync_type;
    int32_t flags;
    float   size;
} mdl_header_t;

static_assert(sizeof (mdl_header_t) == 84, "MDL header is of incorrect size.");

typedef struct mdl_skinsingle
{
    int32_t  is_group;
    uint8_t  data;
} mdl_skinsingle_t;

typedef struct mdl_skingroup
{
    int32_t   is_group;
    int32_t   skin_count;
    float    *time;
    uint8_t **data;
} mdl_skingroup_t;

typedef struct mdl_skin
{
    int32_t is_group;
} mdl_skin_t;

typedef struct mdl_texcoord
{
    int32_t is_seam;
    int32_t u;
    int32_t v;
} mdl_texcoord_t;

typedef struct mdl_triangle
{
    int32_t is_frontfacing;
    int32_t vertices[3];
} mdl_triangle_t;

typedef struct mdl_framevertex
{
    uint8_t pos[3];
    uint8_t normal_index;
} mdl_framevertex_t;

typedef struct mdl_framesingle
{
    int32_t            is_group;
    mdl_framevertex_t  min;
    mdl_framevertex_t  max;
    char name[16];
    mdl_framevertex_t  data;
} mdl_framesingle_t;

typedef struct mdl_framegroup
{
    int32_t            is_group;
    int32_t            frame_count;
    mdl_framevertex_t  min;
    mdl_framevertex_t  max;
    float             *times;
    mdl_framesingle_t *frames;
} mdl_framegroup_t;

typedef struct mdl_frame
{
    int32_t is_group;
} mdl_frame_t;

typedef struct mdl
{
    mdl_header_t    header;
    mdl_skin_t     *skins;
    mdl_texcoord_t *texcoords;
    mdl_triangle_t *triangles;
    mdl_frame_t    *frames;
} mdl_t;

/*
 * Array of RGB values representing the original Quake color palette.
 */
static const uint8_t palette[] = {
    0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x1f, 0x1f, 0x1f, 0x2f, 0x2f, 0x2f,
    0x3f, 0x3f, 0x3f, 0x4b, 0x4b, 0x4b, 0x5b, 0x5b, 0x5b, 0x6b, 0x6b, 0x6b,
    0x7b, 0x7b, 0x7b, 0x8b, 0x8b, 0x8b, 0x9b, 0x9b, 0x9b, 0xab, 0xab, 0xab,
    0xbb, 0xbb, 0xbb, 0xcb, 0xcb, 0xcb, 0xdb, 0xdb, 0xdb, 0xeb, 0xeb, 0xeb,
    0x0f, 0x0b, 0x07, 0x17, 0x0f, 0x0b, 0x1f, 0x17, 0x0b, 0x27, 0x1b, 0x0f,
    0x2f, 0x23, 0x13, 0x37, 0x2b, 0x17, 0x3f, 0x2f, 0x17, 0x4b, 0x37, 0x1b,
    0x53, 0x3b, 0x1b, 0x5b, 0x43, 0x1f, 0x63, 0x4b, 0x1f, 0x6b, 0x53, 0x1f,
    0x73, 0x57, 0x1f, 0x7b, 0x5f, 0x23, 0x83, 0x67, 0x23, 0x8f, 0x6f, 0x23,
    0x0b, 0x0b, 0x0f, 0x13, 0x13, 0x1b, 0x1b, 0x1b, 0x27, 0x27, 0x27, 0x33,
    0x2f, 0x2f, 0x3f, 0x37, 0x37, 0x4b, 0x3f, 0x3f, 0x57, 0x47, 0x47, 0x67,
    0x4f, 0x4f, 0x73, 0x5b, 0x5b, 0x7f, 0x63, 0x63, 0x8b, 0x6b, 0x6b, 0x97,
    0x73, 0x73, 0xa3, 0x7b, 0x7b, 0xaf, 0x83, 0x83, 0xbb, 0x8b, 0x8b, 0xcb,
    0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x0b, 0x0b, 0x00, 0x13, 0x13, 0x00,
    0x1b, 0x1b, 0x00, 0x23, 0x23, 0x00, 0x2b, 0x2b, 0x07, 0x2f, 0x2f, 0x07,
    0x37, 0x37, 0x07, 0x3f, 0x3f, 0x07, 0x47, 0x47, 0x07, 0x4b, 0x4b, 0x0b,
    0x53, 0x53, 0x0b, 0x5b, 0x5b, 0x0b, 0x63, 0x63, 0x0b, 0x6b, 0x6b, 0x0f,
    0x07, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x17, 0x00, 0x00, 0x1f, 0x00, 0x00,
    0x27, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x37, 0x00, 0x00, 0x3f, 0x00, 0x00,
    0x47, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x57, 0x00, 0x00, 0x5f, 0x00, 0x00,
    0x67, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x77, 0x00, 0x00, 0x7f, 0x00, 0x00,
    0x13, 0x13, 0x00, 0x1b, 0x1b, 0x00, 0x23, 0x23, 0x00, 0x2f, 0x2b, 0x00,
    0x37, 0x2f, 0x00, 0x43, 0x37, 0x00, 0x4b, 0x3b, 0x07, 0x57, 0x43, 0x07,
    0x5f, 0x47, 0x07, 0x6b, 0x4b, 0x0b, 0x77, 0x53, 0x0f, 0x83, 0x57, 0x13,
    0x8b, 0x5b, 0x13, 0x97, 0x5f, 0x1b, 0xa3, 0x63, 0x1f, 0xaf, 0x67, 0x23,
    0x23, 0x13, 0x07, 0x2f, 0x17, 0x0b, 0x3b, 0x1f, 0x0f, 0x4b, 0x23, 0x13,
    0x57, 0x2b, 0x17, 0x63, 0x2f, 0x1f, 0x73, 0x37, 0x23, 0x7f, 0x3b, 0x2b,
    0x8f, 0x43, 0x33, 0x9f, 0x4f, 0x33, 0xaf, 0x63, 0x2f, 0xbf, 0x77, 0x2f,
    0xcf, 0x8f, 0x2b, 0xdf, 0xab, 0x27, 0xef, 0xcb, 0x1f, 0xff, 0xf3, 0x1b,
    0x0b, 0x07, 0x00, 0x1b, 0x13, 0x00, 0x2b, 0x23, 0x0f, 0x37, 0x2b, 0x13,
    0x47, 0x33, 0x1b, 0x53, 0x37, 0x23, 0x63, 0x3f, 0x2b, 0x6f, 0x47, 0x33,
    0x7f, 0x53, 0x3f, 0x8b, 0x5f, 0x47, 0x9b, 0x6b, 0x53, 0xa7, 0x7b, 0x5f,
    0xb7, 0x87, 0x6b, 0xc3, 0x93, 0x7b, 0xd3, 0xa3, 0x8b, 0xe3, 0xb3, 0x97,
    0xab, 0x8b, 0xa3, 0x9f, 0x7f, 0x97, 0x93, 0x73, 0x87, 0x8b, 0x67, 0x7b,
    0x7f, 0x5b, 0x6f, 0x77, 0x53, 0x63, 0x6b, 0x4b, 0x57, 0x5f, 0x3f, 0x4b,
    0x57, 0x37, 0x43, 0x4b, 0x2f, 0x37, 0x43, 0x27, 0x2f, 0x37, 0x1f, 0x23,
    0x2b, 0x17, 0x1b, 0x23, 0x13, 0x13, 0x17, 0x0b, 0x0b, 0x0f, 0x07, 0x07,
    0xbb, 0x73, 0x9f, 0xaf, 0x6b, 0x8f, 0xa3, 0x5f, 0x83, 0x97, 0x57, 0x77,
    0x8b, 0x4f, 0x6b, 0x7f, 0x4b, 0x5f, 0x73, 0x43, 0x53, 0x6b, 0x3b, 0x4b,
    0x5f, 0x33, 0x3f, 0x53, 0x2b, 0x37, 0x47, 0x23, 0x2b, 0x3b, 0x1f, 0x23,
    0x2f, 0x17, 0x1b, 0x23, 0x13, 0x13, 0x17, 0x0b, 0x0b, 0x0f, 0x07, 0x07,
    0xdb, 0xc3, 0xbb, 0xcb, 0xb3, 0xa7, 0xbf, 0xa3, 0x9b, 0xaf, 0x97, 0x8b,
    0xa3, 0x87, 0x7b, 0x97, 0x7b, 0x6f, 0x87, 0x6f, 0x5f, 0x7b, 0x63, 0x53,
    0x6b, 0x57, 0x47, 0x5f, 0x4b, 0x3b, 0x53, 0x3f, 0x33, 0x43, 0x33, 0x27,
    0x37, 0x2b, 0x1f, 0x27, 0x1f, 0x17, 0x1b, 0x13, 0x0f, 0x0f, 0x0b, 0x07,
    0x6f, 0x83, 0x7b, 0x67, 0x7b, 0x6f, 0x5f, 0x73, 0x67, 0x57, 0x6b, 0x5f,
    0x4f, 0x63, 0x57, 0x47, 0x5b, 0x4f, 0x3f, 0x53, 0x47, 0x37, 0x4b, 0x3f,
    0x2f, 0x43, 0x37, 0x2b, 0x3b, 0x2f, 0x23, 0x33, 0x27, 0x1f, 0x2b, 0x1f,
    0x17, 0x23, 0x17, 0x0f, 0x1b, 0x13, 0x0b, 0x13, 0x0b, 0x07, 0x0b, 0x07,
    0xff, 0xf3, 0x1b, 0xef, 0xdf, 0x17, 0xdb, 0xcb, 0x13, 0xcb, 0xb7, 0x0f,
    0xbb, 0xa7, 0x0f, 0xab, 0x97, 0x0b, 0x9b, 0x83, 0x07, 0x8b, 0x73, 0x07,
    0x7b, 0x63, 0x07, 0x6b, 0x53, 0x00, 0x5b, 0x47, 0x00, 0x4b, 0x37, 0x00,
    0x3b, 0x2b, 0x00, 0x2b, 0x1f, 0x00, 0x1b, 0x0f, 0x00, 0x0b, 0x07, 0x00,
    0x00, 0x00, 0xff, 0x0b, 0x0b, 0xef, 0x13, 0x13, 0xdf, 0x1b, 0x1b, 0xcf,
    0x23, 0x23, 0xbf, 0x2b, 0x2b, 0xaf, 0x2f, 0x2f, 0x9f, 0x2f, 0x2f, 0x8f,
    0x2f, 0x2f, 0x7f, 0x2f, 0x2f, 0x6f, 0x2f, 0x2f, 0x5f, 0x2b, 0x2b, 0x4f,
    0x23, 0x23, 0x3f, 0x1b, 0x1b, 0x2f, 0x13, 0x13, 0x1f, 0x0b, 0x0b, 0x0f,
    0x2b, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x4b, 0x07, 0x00, 0x5f, 0x07, 0x00,
    0x6f, 0x0f, 0x00, 0x7f, 0x17, 0x07, 0x93, 0x1f, 0x07, 0xa3, 0x27, 0x0b,
    0xb7, 0x33, 0x0f, 0xc3, 0x4b, 0x1b, 0xcf, 0x63, 0x2b, 0xdb, 0x7f, 0x3b,
    0xe3, 0x97, 0x4f, 0xe7, 0xab, 0x5f, 0xef, 0xbf, 0x77, 0xf7, 0xd3, 0x8b,
    0xa7, 0x7b, 0x3b, 0xb7, 0x9b, 0x37, 0xc7, 0xc3, 0x37, 0xe7, 0xe3, 0x57,
    0x7f, 0xbf, 0xff, 0xab, 0xe7, 0xff, 0xd7, 0xff, 0xff, 0x67, 0x00, 0x00,
    0x8b, 0x00, 0x00, 0xb3, 0x00, 0x00, 0xd7, 0x00, 0x00, 0xff, 0x00, 0x00,
    0xff, 0xf3, 0x93, 0xff, 0xf7, 0xc7, 0xff, 0xff, 0xff, 0x9f, 0x5b, 0x53
};

typedef struct mdl_model
{
    /*
     * Number of vertices in the model. Note that this refers to the actual
     * geometry and has no relation to the frame count, so this is the number of
     * vertices per frame.
     */
    int vertex_count;

    /*
     * Number of frames in the model. Currently, the extraction process makes no
     * distinction between normal frames and subframes.
     */
    int frame_count;

    /*
     * Buffer holding all vertex data for all frames of the model. The total
     * size of this buffer is frame_count * vertex_count * sizeof *frames * 3.
     */
    float *frames;

    /*
     * Array of null-terminated strings holding the name of each frame.
     */
    char **frame_names;

    /*
     * Duration of each frame in seconds
     */
    float *frame_durations;

    /*
     * The index of the current or past frame. This is used to provide OpenGL
     * the first set of vertices for interpolation.
     */
    int frame_index;

    int next_frame_index;

    /*
     * The first and last frames of this mode's idle animation. This will play
     * when no other animation is in progress.
     */
    int idle_first;
    int idle_last;

    /*
     * The first and last frames of the model's current animation.
     */
    int anim_first;
    int anim_last;

    /*
     * Determines whether or not the model's current animation should stop
     * (like in death animations) or repeat (like in idle, walking, etc.).
     */
    bool anim_stop;

    int skin_count;
    int skin_width;
    int skin_height;
    int skin_index;
    uint8_t *skins;

    float *texcoords;

    /*
     * OpenGL ID of the buffer object where this model's frames are stored.
     */
    GLuint  vertex_buffer;

    /*
     * An array of OpenGL texture objects containing the skins from this model.
     */
    GLuint *textures;

    /*
     * OpenGL ID of the buffer object where this model's texture coordinates are
     * stored.
     */
    GLuint  texcoord_buffer;

    float position[3];
    float rotation[3];
    float scale[3];
}
model_t;

bool mdl_header_valid(const mdl_header_t * const header)
{
    if (header->magic != MDL_MAGIC) {
        fprintf(stderr, "Incorrect magic number (got %x, should be %x).\n",
                header->magic,
                MDL_MAGIC);
        return false;
    }

    if (header->version != MDL_VERSION) {
        fprintf(stderr, "Incorrect version number (got %d, should be %d).\n",
                header->version,
                MDL_VERSION);
        return false;
    }

    /*
     * If the total number of pixels is not a multiple of 4, data after the
     * first skin will be misaligned. The original Quake engine requires that
     * the skin width be a multiple of 4 (see Mod_LoadAliasModel).
     */
    if ((header->skin_w & 0x03) != 0) {
        fputs("Model skin width is not a multiple of 4.", stderr);
        return false;
    }

    return true;
}

model_t *model_from_mdl(const char *path)
{
    model_t *dest = calloc(1, sizeof *dest);
    
    uint8_t *mdl_data = Utils.readBinaryFile(path);
    if (mdl_data == NULL) {
        perror(path);
        return NULL;
    }

    const mdl_header_t * const header = (mdl_header_t *)mdl_data;
    if (!mdl_header_valid(header)) {
        return NULL;
    }

    const int skin_pixels = header->skin_w * header->skin_h;
    const int skin_bytes = skin_pixels * 4;

    /*
     * Get the real total number of skins, including subskins.
     */
    int total_skins = 0;
    const uint8_t *pos = (uint8_t *)(mdl_data + sizeof *header);
    for (int i = 0; i < header->skin_count; i++) {
        const mdl_skin_t * const skin = (mdl_skin_t *)pos;
        if (skin->is_group) {
            /*
             * TODO: handle this case
             */
            fputs("UNIMPLEMENTED\n", stderr);
            exit(EXIT_FAILURE);
        } else {
            total_skins += 1;
            pos += sizeof skin->is_group + skin_pixels;
        }
    }

    /*
     * This array will contain the RGBA data for every skin, including subskins,
     * laid end-to-end.
     */
    uint8_t *skins = calloc(total_skins * skin_bytes, sizeof *skins);

    /*
     * Iterate through all skins in the MDL data and translate their index data
     * into RGBA format using the Quake palette. 0xff represents a transparent
     * pixel.
     *
     * TODO: There's probably a more elegant way to handle this loop.
     */
    pos = (uint8_t *)(mdl_data + sizeof *header);
    for (int s = 0; s < total_skins; /* s IS NOT INCREMENTED HERE*/) {
        ptrdiff_t skin_offset = skin_bytes * s;
        mdl_skin_t *skin = (mdl_skin_t *)pos;

        if (skin->is_group) {
            /*
             * TODO: handle this case
             */
            fputs("UNIMPLEMENTED\n", stderr);
            exit(EXIT_FAILURE);
        } else {
            /*
             * For a single skin, iterate through its color data and copy the
             * corresponding palette data to the RGBA buffer. Note that the
             * palette data is in RGB format, which means only 3 bytes are
             * copied and then an Alpha value of 0xff is added. If the color
             * index is 0xff, then the copy operation is skipped since the RGBA
             * buffer is initialized to zero on allocation.
             */
            mdl_skinsingle_t *single = (mdl_skinsingle_t *)skin;
            uint8_t *colors = &single->data;
            for (int p = 0; p < skin_pixels; p++) {
                if (colors[p] != 0xff) {
                    ptrdiff_t pixel_offset = 4 * p;
                    memcpy(skins + skin_offset + pixel_offset,
                            palette + 3 * colors[p],
                            3 * sizeof *skins);
                    skins[skin_offset + pixel_offset + 3] = 0xff;
                }
            }
            s += 1;
            pos += sizeof skin->is_group + skin_pixels;
        }
    }

    puts("Skin data extracted.");

    /*
     * Overlay arrays onto the texture coordinates and triangles.
     */
    const mdl_texcoord_t * const mdl_texcoords = (mdl_texcoord_t *)pos;
    pos += header->texcoord_count * sizeof *mdl_texcoords;

    const mdl_triangle_t * const mdl_triangles = (mdl_triangle_t *)pos;
    pos += header->triangle_count * sizeof *mdl_triangles; 

    float *texcoords = calloc(6 * header->triangle_count, sizeof *texcoords);

    /*
     * Compute the model's true texture coordinates by looking up each vertex's
     * index in the texture coordinate table, move the back-facing coordinates
     * to the right half of the texture (where the back is stored), normalize
     * each texture coordinate to the range [0, 1] and store them in the array.
     */
    for (int tri = 0; tri < header->triangle_count; tri++) {
        for (int vert = 0; vert < 3; vert++) {
            const mdl_texcoord_t *tc =
                    &mdl_texcoords[mdl_triangles[tri].vertices[vert]];
            float u = (float)tc->u;
            float v = (float)tc->v;

            if (!mdl_triangles[tri].is_frontfacing && tc->is_seam) {
                u += 0.5f * (float)header->skin_w;
            }

            /*
             * Normalize to [0, 1]
             */
            u = (u + 0.5f) / header->skin_w;
            v = (v + 0.5f) / header->skin_h;

            texcoords[2 * (3 * tri + vert)] = u;
            texcoords[2 * (3 * tri + vert) + 1] = v;
        }
    }

    /*
     * Save this location for when we need to iterate through the frames again
     */
    const uint8_t * const first_frame = pos;

    int total_frames = 0;
    for (int i = 0; i < header->frame_count; i++) {
        const mdl_frame_t * const frame = (mdl_frame_t *)pos;
        if (frame->is_group) {
            printf("frame->is_group value: %x\n", frame->is_group);
            mdl_framegroup_t *group = (mdl_framegroup_t *)frame;
            total_frames += group->frame_count;
            pos = (void *)&group->frames + group->frame_count * (0x18 +
                    header->texcoord_count * 4);
        } else {
            mdl_framesingle_t *single = (mdl_framesingle_t *)frame;
            total_frames += 1;
            pos = (void *)&single->data + header->texcoord_count * 4;
        }
    }
    printf("%d frames in total\n", total_frames);

    float *frame_durations = calloc(total_frames, sizeof *frame_durations);
    float *vertices = calloc(total_frames * 9 * header->triangle_count,
            sizeof *vertices);

    char **frame_names = calloc(total_frames, sizeof *frame_names);

    pos = first_frame;
    for (int f = 0; f < total_frames; /* f IS NOT INCREMENTED HERE*/) {
        mdl_frame_t *frame = (mdl_frame_t *)pos;
        if (frame->is_group) {
            /*
             * TODO: handle this case
             */
            fputs("UNIMPLEMENTED\n", stderr);
            exit(EXIT_FAILURE);
        } else {
            mdl_framesingle_t *single = (mdl_framesingle_t *)frame;
            puts(single->name);
            mdl_framevertex_t *data = &single->data;

            int name_len = strnlen(single->name, 16);
            frame_names[f] = calloc(name_len + 1, sizeof **frame_names);
            strncpy(frame_names[f], single->name, 16);

            for (int tri = 0; tri < header->triangle_count; tri++) {
                for (int vert = 0; vert < 3; vert++) {
                    int32_t tri_index = mdl_triangles[tri].vertices[vert];
                    mdl_framevertex_t *vertex = &data[tri_index];

                    for (int comp = 0; comp < 3; comp++) {
                        vertices[
                            f * 9 * header->triangle_count
                          + 9 * tri + 3 * vert
                          + comp]
                          = header->scale[comp] * vertex->pos[comp] +
                            header->origin[comp];
                    }
                }
            }

            frame_durations[f] = 1.0f / 6.0f;

            f += 1;
            pos = (void *)&single->data + header->texcoord_count * 4;
        }
    }

    dest->frame_count = header->frame_count;
    dest->vertex_count = header->triangle_count * 3;
    dest->frames = vertices;
    dest->frame_names = frame_names;
    dest->frame_durations = frame_durations;
    dest->frame_index = 0;

    dest->skin_count = header->skin_count;
    dest->skin_width = header->skin_w;
    dest->skin_height = header->skin_h;
    dest->skins = skins;
    dest->skin_index = 0;
    dest->texcoords = texcoords;

    return dest;
}

/*
 * Return the size in bytes of one frame in the given model->
 */
int mdl_get_frame_size(const model_t *model)
{
    return model->vertex_count * 3 * sizeof *model->frames;
}

int mdl_get_skin_size(const model_t *model)
{
    return 4 * model->skin_width * model->skin_height * sizeof *model->skins;
}

void model_send_to_opengl(model_t *model)
{
    glGenBuffers(1, &model->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, model->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
            model->frame_count * 3 * model->vertex_count * sizeof *model->frames,
            model->frames,
            GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    model->textures = calloc(model->skin_count, sizeof *model->textures);
    glGenTextures(model->skin_count, model->textures);
    for (int i = 0; i < model->skin_count; i++) {
        glBindTexture(GL_TEXTURE_2D, model->textures[i]);
        vec4_t border_color = { 0.0f, 0.0f, 0.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, model->skin_width,
                model->skin_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                model->skins + i * mdl_get_skin_size(model));
    }

    glGenBuffers(1, &model->texcoord_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, model->texcoord_buffer);
    glBufferData(GL_ARRAY_BUFFER,
            model->vertex_count * 2 * sizeof *model->texcoords,
            model->texcoords,
            GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void model_draw(const model_t *model)
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, model->vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,
            (void *)(model->frame_index * model->vertex_count * 3 *
                sizeof *model->frames));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
            (void *)(model->next_frame_index * model->vertex_count * 3 *
                sizeof *model->frames));

    glBindBuffer(GL_ARRAY_BUFFER, model->texcoord_buffer);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindTexture(GL_TEXTURE_2D, model->textures[0]);
    glDrawArrays(GL_TRIANGLES, 0, model->vertex_count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void model_inc_frame_index(model_t *model)
{
    puts("Incrementing frame index.");
    model->frame_index = model->next_frame_index;
    model->next_frame_index += 1;
    if (model->next_frame_index >= model->frame_count) {
        puts("Frame index went above frame count, wrapping to zero.");
        model->next_frame_index = 0;
    }
    printf("Frame %d -> %d\n", model->frame_index, model->next_frame_index);
}

void model_dec_frame_index(model_t *model)
{
    puts("Decrementing frame index.");
    model->frame_index = model->next_frame_index;
    model->next_frame_index -= 1;
    if (model->next_frame_index < 0) {
        puts("Frame index went below zero, wrapping to frame count.");
        model->next_frame_index = model->frame_count - 1;
    }
    printf("Frame %d -> %d\n", model->frame_index, model->next_frame_index);
}

void model_set_frame_index(model_t *model, int index)
{
    printf("Setting frame index to %d\n", index);
    if (index >= model->frame_count) {
        fprintf(stderr, "Frame index %d out out of bounds [0, %d]\n",
                index, model->frame_count - 1);
    } else {
        model->frame_index = index;
    }
}

void model_set_idle_animation(model_t *model, int first, int last)
{
    if (first < 0 || first >= model->frame_count ||
            last  < 0 || last  >= model->frame_count) {
        return;
    }

    model->idle_first = first;
    model->idle_last = last;
}

void model_set_animation(model_t *model, int first, int last)
{
}

const struct model_namespace Model = {
    .draw = model_draw,
    .decFrameIndex = model_dec_frame_index,
    .incFrameIndex = model_inc_frame_index,
    .fromMDL = model_from_mdl,
    .sendToOpenGL = model_send_to_opengl,
    .setIdleAnimation = model_set_idle_animation
};
