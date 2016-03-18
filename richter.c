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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine.h"
#include "mdl.h"
#include "vecmath.h"

#define INFO(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, \
        __LINE__, ##__VA_ARGS__)

#define ERROR(M, ...) fprintf(stderr, "[ERROR] (%s:%d) " M "\n", __FILE__, \
        __LINE__, ##__VA_ARGS__)

#define ERROR_OPENGL(M, ...) do {\
    GLenum err;\
    size_t ecnt = 0;\
    while ((err = glGetError()) != GL_NO_ERROR) {\
        fprintf(stderr, "[OPENGL] (%s:%d) " M ": %s (%x)\n", __FILE__,\
                __LINE__, glewGetErrorString(err), err, ##__VA_ARGS__);\
        ecnt += 1;\
    }\
    if (ecnt > 0) {\
        exit(EXIT_FAILURE);\
    }\
} while (0);

const char * const vert_src = 
    "#version 330\n"
    "layout (location = 0) in vec3 in_pos1;\n"
    "layout (location = 1) in vec3 in_pos2;\n"
    "layout (location = 2) in vec2 texcoord;\n"
    "out vec2 Texcoord;\n"
    "uniform mat4 world;\n"
    "uniform mat4 persp;\n"
    "void main()\n"
    "{\n"
    "    Texcoord = texcoord;\n"
    "    vec3 lerp_pos = mix(in_pos1, in_pos2, 0.5);\n"
    "    vec4 model_pos = vec4(lerp_pos.x, lerp_pos.y, lerp_pos.z, 1.0f);\n"
    "    vec4 world_pos = world * model_pos;\n"
    "    gl_Position = persp * world_pos;\n"
    "}\n";

const char * const frag_src =
    "#version 330\n"
    "in vec2 Texcoord;\n"
    "out vec4 color;\n"
    "uniform sampler2D tex;\n"
    "void main()\n"
    "{\n"
    "    color = texture(tex, Texcoord);\n"
    "}\n";


GLuint new_shader(const GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    const GLchar **srcaddr = (const GLchar **)&src;
    glShaderSource(shader, 1, srcaddr, NULL);
    glCompileShader(shader);

    GLint shader_stat;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_stat);
    if (shader_stat == GL_FALSE) {
        GLint loglen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &loglen);

        GLchar *log = calloc(loglen + 1, sizeof (*log));
        glGetShaderInfoLog(shader, loglen, NULL, log);
        fprintf(stderr, "Failed to compile shader:\n%s\n", log);
        free(log);
        shader = 0;
    }

    return shader;
}

GLuint new_program(size_t shader_count, const GLuint *shaders)
{
    GLuint program = glCreateProgram();
    ERROR_OPENGL("Couldn't create a new shader program.");

    for (size_t i = 0; i < shader_count; i++) {
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);

    INFO("Linked shaders.");

    GLint prog_stat;
    glGetProgramiv(program, GL_LINK_STATUS, &prog_stat);
    if (prog_stat == GL_FALSE) {
        GLint loglen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &loglen);

        GLchar *log = calloc(loglen + 1, sizeof (*log));
        glGetProgramInfoLog(program, loglen, NULL, log);
        fprintf(stderr, "Failed to link program:\n%s\n", log);
        free(log);
    }

    return program;
}

bool right_pressed, left_pressed;

void handle_keys(GLFWwindow *window, int key, int code, int action, int mods)
{
    switch (action) {
    case GLFW_PRESS:
        switch (key) {
        case GLFW_KEY_RIGHT:
            right_pressed = true;
            break;
        case GLFW_KEY_LEFT:
            left_pressed = true;
            break;
        default:
            break;
        }
        break;
    case GLFW_RELEASE:
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s [mdl-file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    model_t *model = Model.fromMDL(argv[1]);
    if (model == NULL) {
        fputs("MDL read failed.\n", stderr);
        exit(EXIT_FAILURE);
    }

    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 16);
    GLFWwindow *window = glfwCreateWindow(1360, 768, "mdlview", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    glEnable(GL_MULTISAMPLE);

    GLuint vert = new_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = new_shader(GL_FRAGMENT_SHADER, frag_src);
    GLuint prog = new_program(2, (GLuint []){vert, frag});
    ERROR_OPENGL("Shader compilation and linking failed.");
    glUseProgram(prog);

    GLuint world_unif = glGetUniformLocation(prog, "world");
    ERROR_OPENGL("");

    GLuint persp_unif = glGetUniformLocation(prog, "persp");
    mat4_t persp;
    mat4_perspective(persp, 90.0f, 16.0f / 9.0f, 1.f, 1024.f);
    glUniformMatrix4fv(persp_unif, 1, GL_FALSE, (GLfloat *)persp);
    ERROR_OPENGL("");

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    Model.sendToOpenGL(model);
    Model.setIdleAnimation(model, 0, 4);

    glfwSetKeyCallback(window, handle_keys);

    float start, end;
    while (!glfwWindowShouldClose(window)) {
        start = glfwGetTime();
        glfwPollEvents();

        mat4_t world;
        mat4_identity(world);
        mat4_translate_in_place(world, 0.f, -10.f, -50.0f);
        mat4_rotate_X(world, world, -M_PI / 2.0f);
        mat4_rotate_Z(world, world, -M_PI / 2.0f);
        glUniformMatrix4fv(world_unif, 1, GL_FALSE, (GLfloat *)world);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Model.draw(model);

        ERROR_OPENGL("");
        glfwSwapBuffers(window);
        end = glfwGetTime();
        Engine.setTimeDelta(end - start);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
