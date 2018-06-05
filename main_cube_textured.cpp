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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#include "esUtil.h"
#include "matrix.h"

static struct timeval now = { 0, 0 }, then = { 0, 0 };
double elapsed, dnow, dthen;
int fps = 0;

static const GLchar *vertex_shader_source = 
	"attribute vec4 position;\n" \
	"attribute vec2 texcoord;\n" \
	"varying vec2 vtexcoord;\n" \
	"uniform mat4 mvp;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"  gl_Position = position * mvp;\n" \
	"  vtexcoord = texcoord;\n" \
	"}\n";

static const GLchar *fragment_shader_source = 
	"precision mediump float;\n" \
	"varying vec2 vtexcoord;\n" \
	"uniform sampler2D tex;\n" \
	"\n" \
	"void main()\n" \
	"{\n" \
	"  gl_FragColor = texture2D(tex, vtexcoord);\n" \
	"}\n";

static GLfloat vertices[] = {
	/* front */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	/* back */
	-0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* left */
	-0.5f, -0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
	/* right */
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	/* top */
	-0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* bottom */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
};

static const GLfloat uv[] = {
	/* front */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* back */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* left */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* right */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* top */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* bottom */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const GLushort indices[] = {
	/* front */
	 0,  1,  2,
	 0,  2,  3,
	/* back */
	 4,  5,  6,
	 4,  6,  7,
	/* left */
	 8,  9, 10,
	 8, 10, 11,
	/* right */
	12, 13, 14,
	12, 14, 15,
	/* top */
	16, 17, 18,
	16, 18, 19,
	/* bottom */
	20, 21, 22,
	20, 22, 23,
};

static GLfloat x = 0.0f;
static GLfloat y = 0.0f;
static GLfloat z = 0.0f;
static GLint mvp;



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


  EGLint major, minor, n;
  GLuint vertex_shader, fragment_shader;
  GLint ret;
  GLuint program;

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

  program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &ret);
  if (!ret) {
    char *log;

    printf("program linking failed!:\n");
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

    if (ret > 1) {
      log = (char *)malloc(ret);
      glGetProgramInfoLog(program, ret, NULL, log);
      printf("%s", log);
    }

    return -1;
  }

 	glUseProgram(program);
 
        GLint position, texcoord, tex;
	GLuint texid;

        position = glGetAttribLocation(program, "position");
	texcoord = glGetAttribLocation(program, "texcoord");
	tex = glGetUniformLocation(program, "tex");
	mvp = glGetUniformLocation(program, "mvp");



	glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), vertices);
	glEnableVertexAttribArray(position);

	glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE,
			      2 * sizeof(GLfloat), uv);
	glEnableVertexAttribArray(texcoord);

        // load the texture
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	if (!LoadTexture())
        { 
           printf("Failed load the texture.\n");
           exit(1);
        }

	glUniform1i(tex, 0);

  int i = 0;

  for (;;) {

    if (processEvents())
      break;

	glUseProgram(program);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	struct mat4 matrix, modelview, projection, transform, result;

	GLfloat aspect;

	aspect = getWidth() / (GLfloat)getHeight();


	mat4_perspective(&projection, 35.0f, aspect, 1.0f, 1024.0f);
	mat4_identity(&modelview);

	mat4_rotate_x(&transform, x);
	mat4_multiply(&result, &modelview, &transform);
	mat4_rotate_y(&transform, y);
	mat4_multiply(&modelview, &result, &transform);
	mat4_rotate_z(&transform, z);
	mat4_multiply(&result, &modelview, &transform);
	mat4_translate(&transform, 0.0f, 0.0f, -2.0f);
	mat4_multiply(&modelview, &transform, &result);

	mat4_multiply(&matrix, &projection, &modelview);
	glUniformMatrix4fv(mvp, 1, GL_FALSE, (GLfloat *)&matrix);


	glViewport(0, 0, getWidth(), getHeight());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.7f, 0.7f, 0.7f, 0.0f);

	glUniform1i(tex, 0);
	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, texid);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT,
		       indices);

	x += 0.3f;
	y += 0.3f;
	z += 0.4f;
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
