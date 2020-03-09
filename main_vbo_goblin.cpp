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

#include "sbm.h"
#include "obj.h"
#include "vecmath.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

class RenderState 
{
public:
    RenderState() : po(0), vertLoc(0), mvpLoc(0), normalLoc(0), texcoordLoc(0), texUnitLoc(0)
    {}
    ~RenderState() {}

    GLint po;
    GLint vertLoc;
    GLint mvpLoc;
    GLint lightLoc;
    GLint normalLoc;
    GLint texcoordLoc;
    GLint texUnitLoc;

    GLfloat yaw;
    GLfloat pitch;

    OBJObject            ninja;
    GLuint              ninjaTex[1];

};

static RenderState rs;

GLfloat vWhite[] = { 1.0, 1.0, 1.0, 1.0 };

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


using namespace std;

bool LoadTexture()
{
    FILE*	pFile;
	BMPInfo *pBitmapInfo = NULL;
	unsigned long lInfoSize = 0;
	unsigned long lBitSize = 0;
	GLbyte *pBits = NULL;					
	BMPHeader	bitmapHeader;

    pFile = fopen("goblin.bmp", "rb");
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

GLboolean CreateProgram()
{
    const GLchar* vsSource =
      "uniform mat4 mvpMatrix;	\n"
      "\n"
      "attribute vec4 vertPosition;	\n"
      "attribute vec3 normal0;	\n"
      "attribute vec2 texCoord0;	\n"
      "\n"
      "varying vec2 vTexCoord;"
      "varying vec3 vNormal;"
      "\n"
      "void main()"
      "{"
      "   gl_Position = mvpMatrix * vertPosition;"
      "   vTexCoord   = texCoord0;"
      "   vNormal     = normal0;"
      "}";
    const GLchar* fsSource =
      "precision mediump float;	\n"
      "\n"
      "uniform vec4 lightVec;	\n"
      "uniform sampler2D textureUnit0;	\n"
      "\n"
      "varying vec2 vTexCoord;	\n"
      "varying vec3 vNormal;	\n"
      "\n"
      "void main()	\n"
      "{	\n"
      "  vec4 diff = vec4(dot(lightVec.xyz, normalize(vNormal)), dot(lightVec.xyz, normalize(vNormal)), dot(lightVec.xyz, normalize(vNormal)),1);"
      "  gl_FragColor =  diff * texture2D(textureUnit0, vTexCoord);	\n"
      "}	\n";

    GLint status;

    // create and compile the vertex shader
    GLuint vs;
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
	char *log;
        GLint ret;
	glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &ret);
	if (ret>1) {
		log = (char *)malloc(ret);
		glGetShaderInfoLog(vs, ret, NULL, log);
		printf("%s", log);
	}

        printf("Failed to create a vertex shader.\n");
        glDeleteShader(vs);
        return GL_FALSE;
    }

    // create and compile the fragment shader
    GLuint fs;
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
       char *log;
        GLint ret;
	glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &ret);
	if (ret>1) {
		log = (char *)malloc(ret);
		glGetShaderInfoLog(fs, ret, NULL, log);
		printf("%s", log);
	}


	  printf("Failed to create a fragment shader.\n");
        glDeleteShader(vs);
        glDeleteShader(fs);
        return GL_FALSE;
    }

    // create the program and attach the shaders
    GLuint po;
    po = glCreateProgram();
    if (po == 0)
    {
        printf("Failed to create a program.\n");
        return GL_FALSE;
    }
    glAttachShader(po, vs);
    glAttachShader(po, fs);

    // link the program
    glLinkProgram(po);
    glGetProgramiv(po, GL_LINK_STATUS, &status);
    if(!status) 
    {
	char *log;
        GLint ret;
	glGetProgramiv(po, GL_INFO_LOG_LENGTH, &ret);
	if (ret>1) {
		log = (char *)malloc(ret);
		glGetProgramInfoLog(po, ret, NULL, log);
		printf("%s", log);
	}

        printf("Failed to link program.\n");
        glDeleteProgram(po);
        glDeleteShader(vs);
        glDeleteShader(fs);
        return GL_FALSE;
    }

    // Free up no longer needed shader resources
    //glDeleteShader(vs);
    //glDeleteShader(fs);

    rs.po = po;
    rs.vertLoc     = glGetAttribLocation( rs.po, "vertPosition" );
    rs.normalLoc   = glGetAttribLocation( rs.po, "normal0" );
    rs.texcoordLoc = glGetAttribLocation( rs.po, "texCoord0" );
    rs.mvpLoc      = glGetUniformLocation( rs.po, "mvpMatrix" );
    rs.lightLoc    = glGetUniformLocation( rs.po, "lightVec" );
    rs.texUnitLoc  = glGetUniformLocation( rs.po, "textureUnit0" );

    //printf("vertLoc is %d\n", ctx.rs.vertLoc);
    assert(rs.vertLoc >= 0);
    //printf("normalLoc is %d\n", ctx.rs.normalLoc);
    assert(rs.normalLoc >= 0);
    //printf("textCoordLoc is %d\n", ctx.rs.texcoordLoc);
    assert(rs.texcoordLoc >= 0);
    //printf("mvpLoc is %d\n", ctx.rs.mvpLoc);
    assert(rs.mvpLoc >= 0);
    //printf("lightLoc is %d\n", ctx.rs.lightLoc);
    assert(rs.lightLoc >= 0);
    //printf("textUnitLoc is %d\n", ctx.rs.texUnitLoc);
    assert(rs.texUnitLoc >= 0);
    return GL_TRUE;
}

void Dump()
{
    OBJObject* ninja = &rs.ninja;
    GLuint arraystart = ninja->GetFirstFrameVertex(0);
    GLuint arraycount = ninja->GetFrameVertexCount(0);

    // get vertex data
    GLint posAttrib = rs.vertLoc;
    GLint normAttrib = rs.normalLoc;
    GLint uvAttrib = rs.texcoordLoc;
    int posSize = ninja->GetAttribComponents(0);
    int normSize = ninja->GetAttribComponents(1);
    int uvSize = ninja->GetAttribComponents(2);

    unsigned char * data_pointer = ninja->GetVertexData();
    GLuint numverts = ninja->GetNumVertices();
    void* posPtr = data_pointer;

    float *f = (float *)posPtr;
    fprintf(stderr, "triagles\n");
    for (int x=0; x<numverts; x++) {
	
	fprintf(stderr, "%d %f %f %f %f\n", x, *(f), *(f+1), *(f+2), *(f+3));
	f+=4;
    }
    fprintf(stderr, "normals\n");
    for (int x=0; x<numverts; x++) {
	
	fprintf(stderr, "%d %f %f %f\n", x, *(f), *(f+1), *(f+2));
	f+=3;
    }
    fprintf(stderr, "uvs\n");
    for (int x=0; x<numverts; x++) {
	
	fprintf(stderr, "%d %f %f\n", x, *(f), *(f+1));
	f+=2;
    }
    data_pointer += posSize * sizeof(GLfloat) * numverts;
    void* normPtr = data_pointer;
    data_pointer += normSize * sizeof(GLfloat) * numverts;
    void* uvPtr = data_pointer;
	
}

GLuint vbo;

void InitBuffer()
{
    glGenBuffers(1, &vbo);
    //glGenBuffers(1, &vbi);

    // get model properties
    GLuint texture = rs.ninjaTex[0];
    OBJObject* ninja = &rs.ninja;
    GLuint arraystart = ninja->GetFirstFrameVertex(0);
    GLuint arraycount = ninja->GetFrameVertexCount(0);

    // get vertex data
    GLint posAttrib = rs.vertLoc;
    GLint normAttrib = rs.normalLoc;
    GLint uvAttrib = rs.texcoordLoc;
    int posSize = ninja->GetAttribComponents(0);
    int normSize = ninja->GetAttribComponents(1);
    int uvSize = ninja->GetAttribComponents(2);

    unsigned char * data_pointer = ninja->GetVertexData();
    GLuint numverts = ninja->GetNumVertices();
    //fprintf(stderr, "numverts: %d\n", numverts);
    void* posPtr = data_pointer;
    
    data_pointer += posSize * sizeof(GLfloat) * numverts;
    void* normPtr = data_pointer;
    data_pointer += normSize * sizeof(GLfloat) * numverts;
    void* uvPtr = data_pointer;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float *buff = (float *)malloc(numverts * 9 * sizeof(GLfloat));

    float *posF = (float *)posPtr;
    float *normF = (float *)normPtr;
    float *uvF = (float *)uvPtr;

    for (int x=0; x<numverts; x++) {
      buff[x*9+0] = *posF; posF++;
      buff[x*9+1] = *posF; posF++;
      buff[x*9+2] = *posF; posF++;
      buff[x*9+3] = *posF; posF++;

      buff[x*9+4] = *normF; normF++;
      buff[x*9+5] = *normF; normF++;
      buff[x*9+6] = *normF; normF++;

      buff[x*9+7] = *uvF; uvF++;
      buff[x*9+8] = *uvF; uvF++;

    }
    glBufferData(GL_ARRAY_BUFFER, numverts * 9 * sizeof(GLfloat), buff, GL_STATIC_DRAW);

     glBindBuffer(GL_ARRAY_BUFFER, 0);
	fprintf(stderr, "num verts: %d\n", numverts);
}

void Render()
{
    // get program object
    GLuint po = rs.po;
    GLuint texture = rs.ninjaTex[0];

    OBJObject* ninja = &rs.ninja;
    GLuint arraystart = ninja->GetFirstFrameVertex(0);
    GLuint arraycount = ninja->GetFrameVertexCount(0);

    // get vertex data
    GLint posAttrib = rs.vertLoc;
    GLint normAttrib = rs.normalLoc;
    GLint uvAttrib = rs.texcoordLoc;

    // calculate the view matrix from the pitch and yaw of mouse movements
    vec4 target = vec4(0,40,0,0);
    vec4 scale = vec4(1.0,1.0,1.0,0.0);
    mat4 yawmtx(mat4::rotate(rs.yaw, vec4(0,1,0,0)));
    mat4 pitchmtx(mat4::rotate(rs.pitch, vec4(1,0,0,0)));
    vec4 eye = target + (yawmtx * pitchmtx*vec4(0,0,200,1)*scale);
    mat4 view(mat4::lookAt(eye, target,vec4(0,1,0,0)));
    // calculate the projection matrix
    mat4 proj(mat4::perspective(60, ((float) getWidth())/getHeight(), 1, 1000));


    // calvulate the view-projection matrix
    mat4 mvp = proj * view;
    // the light vector is the normalized direction vector pointing
    // from the eye to the origin.
    vec4 light = vec4::normalize(vec4(eye.x, eye.y, eye.z, 0));

    glClearColor ( 0.7f, 0.7f, 0.7f, 0.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // bind the program
    glUseProgram(po);
    glUniform4fv(rs.lightLoc, 1, &light.x);
    glUniformMatrix4fv(rs.mvpLoc, 1, GL_FALSE, &mvp.x.x);
    // the sampler should use texture unit 0
    glUniform1i(rs.texUnitLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    int posSize = 4;
    int normSize = 3;
    int uvSize = 2;

    // set vertex pointers
    GLsizei stride =  9*sizeof(GLfloat);
    glVertexAttribPointer(posAttrib, posSize, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(normAttrib, normSize, GL_FLOAT, GL_FALSE, stride, (const void *)(4*sizeof(GLfloat)));
    glEnableVertexAttribArray(normAttrib);
    glVertexAttribPointer(uvAttrib, uvSize, GL_FLOAT, GL_FALSE, stride, (const void *)(7*sizeof(GLfloat)));
    glEnableVertexAttribArray(uvAttrib);
    // draw
    glDrawArrays(GL_TRIANGLES, arraystart, arraycount);

    // clean up state
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);
    // flip the visible buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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

  CreateProgram();
 rs.ninja.LoadFromOBJ("goblin.obj");
//Dump();
    // load the texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, rs.ninjaTex);
    glBindTexture(GL_TEXTURE_2D, rs.ninjaTex[0]);
    if (!LoadTexture())
    {
        printf("Failed load the texture.\n");
	exit(1);
    }
InitBuffer();
  for (;;) {

    if (processEvents())
      break;

    Render();

	rs.yaw+=2;

    swapBuffers();

    //usleep(8500);

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
