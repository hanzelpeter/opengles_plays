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

#include <math.h>

static struct timeval now = { 0, 0 }, then = { 0, 0 };
double elapsed, dnow, dthen;
int fps = 0;

static struct {
  GLint phase_loc;
  GLint offset_loc;
  GLint position_loc;
  GLuint program;
} gl;

static const char *vertex_shader_source =
"                                        \
   attribute vec4        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
                                         \
   void main()                           \
   {                                     \
      gl_Position = position + offset;   \
      pos = position.xy;                 \
   }                                     \
                                         \n";

static const char *fragment_shader_source =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
      gl_FragColor.a = 1.0;                           \
   }                                                   \
                                                       \n";

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

  //eglConfigurationsInfo("configs.txt");

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

  gl.position_loc = glGetAttribLocation(gl.program, "position");
  gl.phase_loc = glGetUniformLocation(gl.program, "phase");
  gl.offset_loc  = glGetUniformLocation(gl.program, "offset");

  glViewport(0, 0, getWidth(), getHeight());

  for (;;) {

    if (processEvents())
      break;

    static const GLfloat vertexArray[] = {
      +0.0f, +0.5f, +0.0f,
      -0.5f, +0.0f, +0.0f, 
      +0.0f, -0.5f, +0.0f,
      +0.5f, +0.0f, +0.0f, 
      +0.0f, +0.5f, +0.0f
    };

    static float phase = 0;

    /* clear the color buffer */
    glClearColor(0.5, 0.5, 0.5, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniform1f(gl.phase_loc, phase);
    phase = fmodf(phase + 0.4f, 2.f*3.141f);    

    glUniform4f(gl.offset_loc, 0, 0, 0, 0);
    glVertexAttribPointer(gl.position_loc, 3, GL_FLOAT, GL_FALSE, 0, vertexArray);
    glEnableVertexAttribArray(gl.position_loc);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 5);

    swapBuffers();

    // usleep(1500);

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
