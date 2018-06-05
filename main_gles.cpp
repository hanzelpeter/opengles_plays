#include <assert.h>
#include <EGL/egl.h>

#include "f_native.h"
#include "f_egl.h"

#include <GLES2/gl2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "esUtil.h"

static struct timeval now = { 0, 0 }, then = { 0, 0 };
double elapsed, dnow, dthen;
int fps = 0;

static struct {
  GLuint program;
  GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
} gl;

static const char *vertex_shader_source =
    "uniform mat4 modelviewMatrix;      \n"
    "uniform mat4 modelviewprojectionMatrix;\n"
    "uniform mat3 normalMatrix;         \n"
    "                                   \n"
    "attribute vec4 in_position;        \n"
    "attribute vec3 in_normal;          \n"
    "attribute vec4 in_color;           \n"
    "\n"
    "vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);\n"
    "                                   \n"
    "varying vec4 vVaryingColor;        \n"
    "                                   \n"
    "void main()                        \n"
    "{                                  \n"
    "    gl_Position = modelviewprojectionMatrix * in_position;\n"
    "    vec3 vEyeNormal = normalMatrix * in_normal;\n"
    "    vec4 vPosition4 = modelviewMatrix * in_position;\n"
    "    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;\n"
    "    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);\n"
    "    float diff = max(0.0, dot(vEyeNormal, vLightDir));\n"
    "    vVaryingColor = vec4(diff * in_color.rgb, 1.0);\n"
    "}                                  \n";

static const char *fragment_shader_source =
    "precision mediump float;           \n"
    "                                   \n"
    "varying vec4 vVaryingColor;        \n"
    "                                   \n"
    "void main()                        \n"
    "{                                  \n"
    "    gl_FragColor = vVaryingColor;  \n"
    "}                                  \n";

int main(void) {
  
  int alpha;
#ifdef FBDEV
  alpha = 0;
#else
  alpha = 8;
#endif

  if (!initNativeDisplay())
    return -1;
  
  if (!initNativeWindow())
    return -1;

  if (!initEGL(EGL_OPENGL_ES_API, alpha))
    return -1;

  eglConfigurationsInfo("configs.txt");

  EGLint major, minor, n;
  GLuint vertex_shader, fragment_shader;
  GLint ret;

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
  if (!ret) {
    char *log;

    printf("vertex shader compilation failed!:\n");
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);
    if (ret > 1) {
      log = (char *)malloc(ret);
      glGetShaderInfoLog(vertex_shader, ret, NULL, log);
      printf("%s", log);
    }

    return -1;
  }

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
  if (!ret) {
    char *log;

    printf("fragment shader compilation failed!:\n");
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

    if (ret > 1) {
      log = (char *)malloc(ret);
      glGetShaderInfoLog(fragment_shader, ret, NULL, log);
      printf("%s", log);
    }

    return -1;
  }

  gl.program = glCreateProgram();

  glAttachShader(gl.program, vertex_shader);
  glAttachShader(gl.program, fragment_shader);

  glBindAttribLocation(gl.program, 0, "in_position");
  glBindAttribLocation(gl.program, 1, "in_normal");
  glBindAttribLocation(gl.program, 2, "in_color");

  glLinkProgram(gl.program);

  glGetProgramiv(gl.program, GL_LINK_STATUS, &ret);
  if (!ret) {
    char *log;

    printf("program linking failed!:\n");
    glGetProgramiv(gl.program, GL_INFO_LOG_LENGTH, &ret);

    if (ret > 1) {
      log = (char *)malloc(ret);
      glGetProgramInfoLog(gl.program, ret, NULL, log);
      printf("%s", log);
    }

    return -1;
  }

  glUseProgram(gl.program);

  gl.modelviewmatrix = glGetUniformLocation(gl.program, "modelviewMatrix");
  gl.modelviewprojectionmatrix =
      glGetUniformLocation(gl.program, "modelviewprojectionMatrix");
  gl.normalmatrix = glGetUniformLocation(gl.program, "normalMatrix");

  glViewport(0, 0, getWidth(), getHeight());

  int i = 0;

  for (;;) {

    if (processEvents())
      break;

    ESMatrix modelview;
    static const GLfloat vVertices[] = {
      // front
      -1.0f, -1.0f, +1.0f, // point blue
      +1.0f, -1.0f, +1.0f, // point magenta
      -1.0f, +1.0f, +1.0f, // point cyan
      +1.0f, +1.0f, +1.0f, // point white
      // back
      +1.0f, -1.0f, -1.0f, // point red
      -1.0f, -1.0f, -1.0f, // point black
      +1.0f, +1.0f, -1.0f, // point yellow
      -1.0f, +1.0f, -1.0f, // point green
      // right
      +1.0f, -1.0f, +1.0f, // point magenta
      +1.0f, -1.0f, -1.0f, // point red
      +1.0f, +1.0f, +1.0f, // point white
      +1.0f, +1.0f, -1.0f, // point yellow
      // left
      -1.0f, -1.0f, -1.0f, // point black
      -1.0f, -1.0f, +1.0f, // point blue
      -1.0f, +1.0f, -1.0f, // point green
      -1.0f, +1.0f, +1.0f, // point cyan
      // top
      -1.0f, +1.0f, +1.0f, // point cyan
      +1.0f, +1.0f, +1.0f, // point white
      -1.0f, +1.0f, -1.0f, // point green
      +1.0f, +1.0f, -1.0f, // point yellow
      // bottom
      -1.0f, -1.0f, -1.0f, // point black
      +1.0f, -1.0f, -1.0f, // point red
      -1.0f, -1.0f, +1.0f, // point blue
      +1.0f, -1.0f, +1.0f  // point magenta
    };

    static const GLfloat vColors[] = {
      // front
      0.0f, 0.0f, 1.0f, // blue
      1.0f, 0.0f, 1.0f, // magenta
      0.0f, 1.0f, 1.0f, // cyan
      1.0f, 1.0f, 1.0f, // white
      // back
      1.0f, 0.0f, 0.0f, // red
      0.0f, 0.0f, 0.0f, // black
      1.0f, 1.0f, 0.0f, // yellow
      0.0f, 1.0f, 0.0f, // green
      // right
      1.0f, 0.0f, 1.0f, // magenta
      1.0f, 0.0f, 0.0f, // red
      1.0f, 1.0f, 1.0f, // white
      1.0f, 1.0f, 0.0f, // yellow
      // left
      0.0f, 0.0f, 0.0f, // black
      0.0f, 0.0f, 1.0f, // blue
      0.0f, 1.0f, 0.0f, // green
      0.0f, 1.0f, 1.0f, // cyan
      // top
      0.0f, 1.0f, 1.0f, // cyan
      1.0f, 1.0f, 1.0f, // white
      0.0f, 1.0f, 0.0f, // green
      1.0f, 1.0f, 0.0f, // yellow
      // bottom
      0.0f, 0.0f, 0.0f, // black
      1.0f, 0.0f, 0.0f, // red
      0.0f, 0.0f, 1.0f, // blue
      1.0f, 0.0f, 1.0f  // magenta
    };

    static const GLfloat vNormals[] = {
      // front
      +0.0f, +0.0f, +1.0f, // forward
      +0.0f, +0.0f, +1.0f, // forward
      +0.0f, +0.0f, +1.0f, // forward
      +0.0f, +0.0f, +1.0f, // forward
      // back
      +0.0f, +0.0f, -1.0f, // backbard
      +0.0f, +0.0f, -1.0f, // backbard
      +0.0f, +0.0f, -1.0f, // backbard
      +0.0f, +0.0f, -1.0f, // backbard
      // right
      +1.0f, +0.0f, +0.0f, // right
      +1.0f, +0.0f, +0.0f, // right
      +1.0f, +0.0f, +0.0f, // right
      +1.0f, +0.0f, +0.0f, // right
      // left
      -1.0f, +0.0f, +0.0f, // left
      -1.0f, +0.0f, +0.0f, // left
      -1.0f, +0.0f, +0.0f, // left
      -1.0f, +0.0f, +0.0f, // left
      // top
      +0.0f, +1.0f, +0.0f, // up
      +0.0f, +1.0f, +0.0f, // up
      +0.0f, +1.0f, +0.0f, // up
      +0.0f, +1.0f, +0.0f, // up
      // bottom
      +0.0f, -1.0f, +0.0f, // down
      +0.0f, -1.0f, +0.0f, // down
      +0.0f, -1.0f, +0.0f, // down
      +0.0f, -1.0f, +0.0f  // down
    };

    /* clear the color buffer */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, vNormals);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, vColors);
    glEnableVertexAttribArray(2);

    esMatrixLoadIdentity(&modelview);
    esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
    esRotate(&modelview, 45.0f + (0.25f * i), 1.0f, 0.0f, 0.0f);
    esRotate(&modelview, 45.0f - (0.5f * i), 0.0f, 1.0f, 0.0f);
    esRotate(&modelview, 10.0f + (0.15f * i), 0.0f, 0.0f, 1.0f);

    GLfloat aspect = (GLfloat)(getHeight()) / (GLfloat)(getWidth());

    ESMatrix projection;
    esMatrixLoadIdentity(&projection);
    esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f,
              10.0f);

    ESMatrix modelviewprojection;
    esMatrixLoadIdentity(&modelviewprojection);
    esMatrixMultiply(&modelviewprojection, &modelview, &projection);

    float normal[9];
    normal[0] = modelview.m[0][0];
    normal[1] = modelview.m[0][1];
    normal[2] = modelview.m[0][2];
    normal[3] = modelview.m[1][0];
    normal[4] = modelview.m[1][1];
    normal[5] = modelview.m[1][2];
    normal[6] = modelview.m[2][0];
    normal[7] = modelview.m[2][1];
    normal[8] = modelview.m[2][2];

    glUniformMatrix4fv(gl.modelviewmatrix, 1, GL_FALSE, &modelview.m[0][0]);
    glUniformMatrix4fv(gl.modelviewprojectionmatrix, 1, GL_FALSE,
                       &modelviewprojection.m[0][0]);
    glUniformMatrix3fv(gl.normalmatrix, 1, GL_FALSE, normal);

    glEnable(GL_CULL_FACE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

    swapBuffers();

    // usleep(1500);

    i++;

    fps++;
    gettimeofday(&now, NULL);

    dnow = now.tv_sec + (now.tv_usec / 1000000.0);
    dthen = then.tv_sec + (then.tv_usec / 1000000.0);
    elapsed = dnow - dthen;
    if (elapsed >= 1.0) {
      memcpy((char *)&then, (char *)&now, sizeof(struct timeval));
      fprintf(stderr, "fps=%d\n", fps);
      fps = 0;
    }
  }

  deinitEGL();
  return 0;
}
