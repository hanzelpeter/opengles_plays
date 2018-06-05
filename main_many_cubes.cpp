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


#pragma pack(1)
struct RGB { 
  GLbyte blue;
  GLbyte green;
  GLbyte red;
  GLbyte alpha;
};

struct BMPInfoHeader {
  GLuint	size;
  GLuint	width;
  GLuint	height;
  GLushort  planes;
  GLushort  bits;
  GLuint	compression;
  GLuint	imageSize;
  GLuint	xScale;
  GLuint	yScale;
  GLuint	colors;
  GLuint	importantColors;
};

struct BMPHeader {
  GLushort	type; 
  GLuint	size; 
  GLushort	unused; 
  GLushort	unused2; 
  GLuint	offset; 
}; 

struct BMPInfo {
  BMPInfoHeader		header;
  RGB				colors[1];
};

#pragma pack(8)


#define g_vsize_pos 3
#define g_stride_pos (g_vsize_pos * sizeof(GLfloat))
#define g_vsize_nor 3
#define g_stride_nor (g_vsize_nor * sizeof(GLfloat))


typedef struct
{
  union
  {
    struct
    {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    };
    uint32_t rgba;
  };
} color_t;

  
typedef struct
{
  float modelproj_mat[16];
  float nor_mat[16];
  uint32_t type;
  color_t color;
  float x;
  float y;
  float z;
  float dx;
  float dy;
  float dz;
  float rx;
  float ry;
  float rz;
  float drx;
  float dry;
  float drz;
} obj_3d_t;


static GLfloat light_pos[4] = {5.0, 5.0, 10.0, 1.0};
static obj_3d_t obj[1024];
static GLfloat proj_mat[16];
static float model_mat[16];
static float modelproj_mat[16];
static float nor_mat[16];
static float col[4];


static GLfloat vtx_cube[] =
{
  1.000000,1.000000,-1.000000,
  1.000000,-1.000000,-1.000000,
  -1.000000,-1.000000,-1.000000,
  1.000000,0.999999,1.000000,
  -1.000000,1.000000,1.000000,
  0.999999,-1.000001,1.000000,
  1.000000,1.000000,-1.000000,
  1.000000,0.999999,1.000000,
  1.000000,-1.000000,-1.000000,
  1.000000,-1.000000,-1.000000,
  0.999999,-1.000001,1.000000,
  -1.000000,-1.000000,-1.000000,
  -1.000000,-1.000000,-1.000000,
  -1.000000,-1.000000,1.000000,
  -1.000000,1.000000,1.000000,
  1.000000,0.999999,1.000000,
  1.000000,1.000000,-1.000000,
  -1.000000,1.000000,1.000000,
  -1.000000,1.000000,-1.000000,
  -1.000000,-1.000000,1.000000,
  1.000000,0.999999,1.000000,
  0.999999,-1.000001,1.000000,
  1.000000,-1.000000,-1.000000,
  -1.000000,-1.000000,1.000000,
  -1.000000,1.000000,-1.000000,
  -1.000000,1.000000,-1.000000,
};


static GLfloat nor_cube[] =
{
  0.000000,0.000000,-1.000000,
  0.000000,0.000000,-1.000000,
  0.000000,0.000000,-1.000000,
  -0.000000,-0.000000,1.000000,
  -0.000000,-0.000000,1.000000,
  -0.000000,-0.000000,1.000000,
  1.000000,0.000000,-0.000000,
  1.000000,0.000000,-0.000000,
  1.000000,0.000000,-0.000000,
  -0.000000,-1.000000,-0.000000,
  -0.000000,-1.000000,-0.000000,
  -0.000000,-1.000000,-0.000000,
  -1.000000,0.000000,-0.000000,
  -1.000000,0.000000,-0.000000,
  -1.000000,0.000000,-0.000000,
  0.000000,1.000000,0.000000,
  0.000000,1.000000,0.000000,
  0.000000,1.000000,0.000000,
  0.000000,0.000000,-1.000000,
  0.000000,-0.000000,1.000000,
  1.000000,-0.000001,0.000000,
  1.000000,-0.000001,0.000000,
  1.000000,-0.000001,0.000000,
  -0.000000,-1.000000,0.000000,
  -1.000000,0.000000,-0.000000,
  0.000000,1.000000,0.000000,
};


static GLushort ind_cube[] =
{
  0,1,2,
  3,4,5,
  6,7,8,
  9,10,11,
  12,13,14,
  15,16,17,
  18,0,2,
  4,19,5,
  20,21,22,
  10,23,11,
  24,12,14,
  16,25,17,
};


static const GLchar *VShader1 = 
  "attribute vec3 pos;\n"
  "attribute vec3 nor;\n"
  "uniform mat4 model_mat;\n"
  "uniform mat4 nor_mat;\n"
  "uniform vec4 light_pos;\n"
  "uniform vec4 col;\n"
  "varying vec4 fcol;\n"
  "void main(void)\n"
  "{\n"
  " vec3 eyenor = normalize(vec3(nor_mat * vec4(nor, 1.0)));\n"
  " vec3 lnor = normalize(light_pos.xyz);\n"
  " float diffuse = max(dot(eyenor, lnor), 0.0);\n"
  " fcol = vec4(vec3(diffuse * col), 1.0) ;\n"
  " gl_Position = model_mat * vec4(pos, 1.0);\n"
  "}\n";


static const GLchar *FShader1 = 
  "precision mediump float;\n"
  "varying vec4 fcol;\n"
  "void main(void)\n"
  "{\n"
  " gl_FragColor = fcol;\n"
  "}\n";


typedef struct
{
  uint32_t screen_width;
  uint32_t screen_height;
  GLuint vshader;
  GLuint fshader;
  GLuint program;
  GLuint vbo_pos;
  GLuint vbo_nor;
  GLuint vbo_ind;
  GLint vloc_pos;
  GLint vloc_nor;
  GLuint uloc_model;
  GLuint uloc_nor;
  GLuint uloc_light;
  GLuint uloc_col;
  GLfloat* vtx_pos;
  GLfloat* vtx_nor;
} graphics_context_t;


static inline float randf(void)
{
  return (float)rand() / (float)RAND_MAX;
}


static inline float get_diff_time(struct timeval start, struct timeval end)
{
  float dt = (float)(end.tv_sec - start.tv_sec) + (float)(end.tv_usec - start.tv_usec) * 0.000001f;
  return dt;
}


GLuint create_vbo(GLenum target, void *vtx, size_t size)
{
  static GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(target, vbo);
  glBufferData(target, size, vtx, GL_STATIC_DRAW);
  glBindBuffer(target, 0);
  return vbo;
}


void release_vbo(GLenum target, GLuint vbo)
{
  glBindBuffer(target, 0);
  glDeleteBuffers(1, &vbo);
}


void init_shader(graphics_context_t *gc, const GLchar *vs, const GLchar *fs)
{
  gc->vshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(gc->vshader, 1, &vs, 0);
  glCompileShader(gc->vshader);
  gc->fshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(gc->fshader, 1, &fs, 0);
  glCompileShader(gc->fshader);

  gc->program = glCreateProgram();
  glAttachShader(gc->program, gc->vshader);
  glAttachShader(gc->program, gc->fshader);
  glLinkProgram(gc->program);
}


void release_shader(graphics_context_t *gc)
{
  glDeleteProgram(gc->program);
  glDeleteShader(gc->vshader);
  glDeleteShader(gc->fshader);
}


void init_gl(graphics_context_t *gc)
{
  init_shader(gc, VShader1, FShader1);
  gc->vbo_pos = create_vbo(GL_ARRAY_BUFFER, vtx_cube, sizeof(vtx_cube));
  gc->vbo_nor = create_vbo(GL_ARRAY_BUFFER, nor_cube, sizeof(nor_cube));
  gc->vbo_ind = create_vbo(GL_ELEMENT_ARRAY_BUFFER, ind_cube, sizeof(ind_cube));
  gc->vloc_pos = glGetAttribLocation(gc->program, "pos");
  gc->vloc_nor = glGetAttribLocation(gc->program, "nor");
  gc->uloc_model = glGetUniformLocation(gc->program, "model_mat");
  gc->uloc_nor = glGetUniformLocation(gc->program, "nor_mat");
  gc->uloc_light = glGetUniformLocation(gc->program, "light_pos");
  gc->uloc_col = glGetUniformLocation(gc->program, "col");

  glBindBuffer(GL_ARRAY_BUFFER, gc->vbo_pos);
  glVertexAttribPointer(gc->vloc_pos, g_vsize_pos, GL_FLOAT, GL_FALSE, g_stride_pos, (GLfloat *)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, gc->vbo_nor);
  glVertexAttribPointer(gc->vloc_nor, g_vsize_nor, GL_FLOAT, GL_FALSE, g_stride_nor, (GLfloat *)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, gc->screen_width, gc->screen_height);
  glUseProgram(gc->program);

  glDisable(GL_BLEND);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}


void release_gl(graphics_context_t *gc)
{
  release_shader(gc);
  release_vbo(GL_ARRAY_BUFFER, gc->vbo_pos);
  release_vbo(GL_ARRAY_BUFFER, gc->vbo_nor);
  release_vbo(GL_ELEMENT_ARRAY_BUFFER, gc->vbo_ind);
}


#include "glsutil.h"

bool LoadTexture()
{
    FILE*	pFile;
	BMPInfo *pBitmapInfo = NULL;
	unsigned long lInfoSize = 0;
	unsigned long lBitSize = 0;
	GLbyte *pBits = NULL;					
	BMPHeader	bitmapHeader;

    pFile = fopen("tegra.bmp", "rb");
    if(pFile == NULL)
        return false;

    fprintf(stderr, "size of BMPheader; %d\n", sizeof(BMPHeader));

    if(fread(&bitmapHeader, 1, sizeof(BMPHeader), pFile) != sizeof(BMPHeader))
    {
        fclose(pFile);
        printf("Failed to load texture.\n");
		return false;
    }

	lInfoSize = bitmapHeader.offset - sizeof(BMPHeader);

	fprintf(stderr, "lInfoSize: %d\n", lInfoSize);

	pBitmapInfo = (BMPInfo *) malloc(sizeof(GLbyte)*lInfoSize);
	if(fread(pBitmapInfo, 1, lInfoSize, pFile) != lInfoSize)
	{
		free(pBitmapInfo);
		fclose(pFile);
        printf("Failed to load texture.\n");
		return false;
	}

	GLint nWidth = pBitmapInfo->header.width;
	GLint nHeight = pBitmapInfo->header.height;
	lBitSize = pBitmapInfo->header.imageSize;

	if(pBitmapInfo->header.bits != 24)
	{
        printf("Failed to load texture.\n");
		free(pBitmapInfo);
		return false;
	}

	if(lBitSize == 0)
		lBitSize = (nWidth *
           pBitmapInfo->header.bits + 7) / 8 *
  		  abs(nHeight);

	free(pBitmapInfo);
	pBits = (GLbyte*)malloc(sizeof(GLbyte)*lBitSize);

	fprintf(stderr, "lBitSize: %d\n", lBitSize);

	if(fread(pBits, 1, lBitSize, pFile) != lBitSize)
	{
		free(pBits);
		pBits = NULL;
	}

	fclose(pFile);

        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pBits);

    return true;
}

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


  static graphics_context_t gc;

  gc.screen_width = getWidth();
  gc.screen_height = getHeight();
  init_gl(&gc);

  float aspect = (float)gc.screen_width / (float)gc.screen_height;

  mat_perspective(proj_mat, aspect, 1.0f, 1024.0f, 60.0f);
  glUniform4fv(gc.uloc_light, 1, light_pos);

  int j;
  for (j = 0; j < 1024; j++)
  {
    obj[j].x = randf() * 60.0f - 30.0f;
    obj[j].y = randf() * 60.0f - 30.0f;
    obj[j].z = randf() * 300.0f - 300.0f;
    obj[j].dx = randf() * 0.0f - 0.00f;
    obj[j].dy = randf() * 0.0f - 0.00f;
    obj[j].dz = randf() * 1.3f - 0.3f;
    if (fabs(obj[j].dz) < 0.01f)
    {
      obj[j].dz = 0.01f;
    }
    obj[j].rx = randf() * 6.28;
    obj[j].ry = randf() * 6.28;
    obj[j].rz = randf() * 6.28;
    obj[j].drx = randf() * 0.1f - 0.05f;
    obj[j].dry = randf() * 0.1f - 0.05f;
    obj[j].drz = randf() * 0.1f - 0.05f;
    obj[j].color.r = (uint8_t)(randf() * 255.5f);
    obj[j].color.g = (uint8_t)(randf() * 255.5f);
    obj[j].color.b = (uint8_t)(randf() * 255.5f);
    obj[j].color.a = (uint8_t)255;
  }

  float x = 0.0f;
  float y = 0.0f;
  float rx = 0.0f;
  float ry = 0.0f;

  int i = 0;

  for (;;) {

    if (processEvents())
      break;

    glUseProgram(gc.program);
    glBindBuffer(GL_ARRAY_BUFFER, gc.vbo_pos);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gc.vbo_ind);
    glEnableVertexAttribArray(gc.vloc_pos);
    glEnableVertexAttribArray(gc.vloc_nor);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (j = 0; j < 240; j++)
    {
      obj[j].x += obj[j].dx;
      obj[j].y += obj[j].dy;
      obj[j].z += obj[j].dz + y;
      if (obj[j].z > 1.0f)
      {
        obj[j].x = randf() * 60.0f - 30.0f;
        obj[j].y = randf() * 60.0f - 30.0f;
        obj[j].z = -300.0f;
      }
      if (obj[j].z < -400.0f)
      {
        obj[j].x = randf() * 60.0f - 30.0f;
        obj[j].y = randf() * 60.0f - 30.0f;
        obj[j].z = 1.0f;
      }
      obj[j].rx += obj[j].drx;
      obj[j].ry += obj[j].dry;
      obj[j].rz += obj[j].drz;

      mat_identity(model_mat);
      mat_translate(model_mat, obj[j].x, obj[j].y, obj[j].z);
      mat_rotate_x(model_mat, obj[j].rx);
      mat_rotate_y(model_mat, obj[j].ry);
      mat_rotate_z(model_mat, obj[j].rz);

      mat_copy(nor_mat, model_mat);
      mat_invert(nor_mat);
      mat_transpose(nor_mat);
      glUniformMatrix4fv(gc.uloc_nor, 1, GL_FALSE, nor_mat);
      mat_copy(obj[j].nor_mat, nor_mat);

      mat_copy(modelproj_mat, proj_mat);
      mat_mul(modelproj_mat, model_mat);
      glUniformMatrix4fv(gc.uloc_model, 1, GL_FALSE, modelproj_mat);
      mat_copy(obj[j].modelproj_mat, modelproj_mat);

      col[0] = (float)obj[j].color.r * 0.00392156862745f;
      col[1] = (float)obj[j].color.g * 0.00392156862745f;
      col[2] = (float)obj[j].color.b * 0.00392156862745f;
      col[3] = (float)obj[j].color.a * 0.00392156862745f;
      glUniform4fv(gc.uloc_col, 1, col);

      glDrawElements(GL_TRIANGLES, sizeof(ind_cube) / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
    }
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
  glDisableVertexAttribArray(gc.vloc_nor);
    glDisableVertexAttribArray(gc.vloc_pos);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  
glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  deinitEGL();
  return 0;
}
