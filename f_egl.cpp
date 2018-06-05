#include <assert.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "f_egl.h"

EGLDisplay eglDisplay;
EGLConfig eglConfig;
EGLSurface eglSurface;
EGLContext eglContext;

extern NativeDisplayType display;
extern NativeWindowType window;

bool initEGL(EGLenum api, int alphaSize) {

  int i32NumConfigs, i32MajorVersion, i32MinorVersion;

  eglDisplay = eglGetDisplay(display);
  eglInitialize(eglDisplay, NULL, NULL);
  assert(eglGetError() == EGL_SUCCESS);

  if (!eglInitialize(eglDisplay, &i32MajorVersion, &i32MinorVersion)) {
    printf("Error: eglInitialize() failed.\n");
    return false;
  }
  eglBindAPI(api);

  static const int ai32ConfigAttribs_VG[] = {
    EGL_RED_SIZE,        8, 
    EGL_GREEN_SIZE,      8,              
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      alphaSize, 
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, 
    EGL_NONE
  };

  static const int ai32ConfigAttribs_GLES[] = {
    EGL_RED_SIZE,        8,
    EGL_GREEN_SIZE,      8,
    EGL_BLUE_SIZE,       8,
    EGL_ALPHA_SIZE,      alphaSize,
    EGL_DEPTH_SIZE, 16,
    EGL_SURFACE_TYPE,    EGL_WINDOW_BIT, 
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  static const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                            EGL_NONE };

  EGLBoolean status;

  if (api == EGL_OPENGL_ES_API)
    status = eglChooseConfig(eglDisplay, ai32ConfigAttribs_GLES, &eglConfig, 1,
                             &i32NumConfigs);
  else if (api == EGL_OPENVG_API)
    status = eglChooseConfig(eglDisplay, ai32ConfigAttribs_VG, &eglConfig, 1,
                             &i32NumConfigs);
  else {
    printf("Unsupported API\n");
    return false;
  }

  if (!status || (i32NumConfigs != 1)) {
    printf("configs: %d\n", i32NumConfigs);
    printf("Error: eglChooseConfig() failed.\n");
    return false;
  }

  eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, window, NULL);
  if ((eglGetError() != EGL_SUCCESS) || (eglSurface == EGL_NO_SURFACE)) {
    printf("Error: eglCreateWindowSurface() failed.\n");
    return false;
  }
  if (api == EGL_OPENGL_ES_API)
    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, context_attribs);
  else
    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL, NULL);

  if ((eglGetError() != EGL_SUCCESS) || (eglContext == EGL_NO_CONTEXT)) {
    printf("Error: eglCreateContext() failed.\n");
    return false;
  }

  eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
  if (eglGetError() != EGL_SUCCESS) {
    printf("Error: eglMakeCurrent() failed.\n");
    return false;
  }

  eglSwapInterval(eglDisplay, 1);

  return true;
}

void swapBuffers() { eglSwapBuffers(eglDisplay, eglSurface); }

void deinitEGL() {
  eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  assert(eglGetError() == EGL_SUCCESS);
  eglTerminate(eglDisplay);
  assert(eglGetError() == EGL_SUCCESS);
  eglReleaseThread();
}

bool eglConfigurationsInfo(const char *fileName) {

  FILE *f = NULL;
  EGLint i;
  EGLConfig *configList;
  EGLint numConfig;
  EGLint dummy;

  if (eglDisplay == EGL_NO_DISPLAY) {
    printf("eglDisplay is EGL_NO_DISPLAY!\n");
    return false;
  }

  if (eglGetConfigs(eglDisplay, NULL, 0, &numConfig) == EGL_FALSE) {
    printf("eglGetConfigs failed!\n");
    return false;
  }

  if (numConfig == 0)
    return true;

  //printf("configs num %d\n", numConfig);

  configList = (EGLConfig *)malloc(sizeof(EGLConfig) * numConfig);
  if (!configList) {
    printf("iMemory allocation used to store EGL configurations failed!\n");
    return false;
  }

  if (eglGetConfigs(eglDisplay, configList, numConfig, &dummy) == EGL_FALSE) {
    printf("eglGetConfigs failed!\n");
    free(configList);
    return false;
  }
  numConfig = dummy;
  //printf("configs num %d\n", numConfig);

  // open the file, if requested
  if (fileName) {
    f = fopen(fileName, "wt");
    if (!f) {
      perror("Cannot create file:");
      free(configList);
      return false;
    }
  }

  // print configurations info
  for (i = 0; i < numConfig; ++i) {

    EGLint bufferSize, redSize, greenSize, blueSize, alphaSize, alphaMaskSize,
        depthSize, stencilSize;
    EGLint configId, samples, sampleBuffers, surfaceType;
    char str[16] = "";

    eglGetConfigAttrib(eglDisplay, configList[i], EGL_BUFFER_SIZE, &bufferSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_RED_SIZE, &redSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_GREEN_SIZE, &greenSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_BLUE_SIZE, &blueSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_ALPHA_SIZE, &alphaSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_ALPHA_MASK_SIZE,
                       &alphaMaskSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_DEPTH_SIZE, &depthSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_STENCIL_SIZE,
                       &stencilSize);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_CONFIG_ID, &configId);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_SAMPLES, &samples);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_SAMPLE_BUFFERS,
                       &sampleBuffers);
    eglGetConfigAttrib(eglDisplay, configList[i], EGL_SURFACE_TYPE,
                       &surfaceType);

    if (surfaceType & EGL_PBUFFER_BIT)
      strcat(str, "PBUF|");
    else
      strcat(str, "    |");
    if (surfaceType & EGL_PIXMAP_BIT)
      strcat(str, "PIXM|");
    else
      strcat(str, "    |");
    if (surfaceType & EGL_WINDOW_BIT)
      strcat(str, "WIND");
    else
      strcat(str, "    ");

    if (!fileName)
      printf("[%8d] %s %2dbits R%1dG%1dB%1dA%1d aMsk%1d D%dS%1d - samples %d, "
             "sample buffers %d\n",
             configId, str, bufferSize, redSize, greenSize, blueSize, alphaSize,
             alphaMaskSize, depthSize, stencilSize, samples, sampleBuffers);
    else
      fprintf(f, "[%8d] %s %2dbits R%1dG%1dB%1dA%1d aMsk%1d D%dS%1d - samples "
                 "%d, sample buffers %d\n",
              configId, str, bufferSize, redSize, greenSize, blueSize,
              alphaSize, alphaMaskSize, depthSize, stencilSize, samples,
              sampleBuffers);
  }
  // free memory and close the file
  free(configList);
  if (f)
    fclose(f);
  return true;
}
