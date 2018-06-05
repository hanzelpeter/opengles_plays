#ifndef F_EGL_H
#define F_EGL_H

bool initEGL(EGLenum api, int alphaSize);
void deinitEGL();
bool eglConfigurationsInfo(const char *fileName);
void swapBuffers();

#endif
