#include <assert.h>
#include <stdio.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

NativeDisplayType display;
NativeWindowType window;

static void exitHandler(int signal);

void setExitHandler()
{
  struct sigaction act;
  act.sa_handler = exitHandler;
  sigemptyset(&act.sa_mask);

  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  sigaction(SIGABRT, &act, NULL);
  sigaction(SIGTSTP, &act, NULL);

  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGILL, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);

}


#ifdef XWINDOW

#include "X11/Xlib.h"
#include "X11/Xutil.h"

#define WINDOW_HEIGHT 600
#define WINDOW_WIDTH 800

Display *x11Display;
Window x11Window;
 
int getWidth() {
  return WINDOW_WIDTH;
};
int getHeight() {
  return WINDOW_HEIGHT;
};

bool initNativeDisplay() {

  x11Display = XOpenDisplay(0);
  if (!x11Display) {
    printf("Error: Unable to open X display\n");
    return false;
  }
  display = (NativeDisplayType)x11Display;
  return true;
}

bool initNativeWindow() {
  x11Window = 0;
  Colormap x11Colormap = 0;

  EGLDisplay eglDisplay = 0;
  EGLConfig eglConfig = 0;
  EGLSurface eglSurface = 0;
  EGLContext eglContext = 0;

  int i32NumConfigs, i32MajorVersion, i32MinorVersion;
  int err;

  Window sRootWindow;
  XSetWindowAttributes sWA;
  unsigned int ui32Mask;
  int i32Depth;

  long x11Screen;
  x11Screen = XDefaultScreen(x11Display);

  // Gets the window parameters
  sRootWindow = RootWindow(x11Display, x11Screen);
  i32Depth = DefaultDepth(x11Display, x11Screen);
  XVisualInfo *x11Visual;
  x11Visual = new XVisualInfo;
  XMatchVisualInfo(x11Display, x11Screen, i32Depth, TrueColor, x11Visual);
  if (!x11Visual) {
    printf("Error: Unable to acquire visual\n");
    return false;
  }

  x11Colormap =
      XCreateColormap(x11Display, sRootWindow, x11Visual->visual, AllocNone);
  sWA.colormap = x11Colormap;

  // Add to these for handling other events
  sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask |
                   ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
  ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

  // Creates the X11 window
  x11Window = XCreateWindow(x11Display, RootWindow(x11Display, x11Screen), 0, 0,
                            WINDOW_WIDTH, WINDOW_HEIGHT, 0, CopyFromParent,
                            InputOutput, CopyFromParent, ui32Mask, &sWA);
  XMapWindow(x11Display, x11Window);
  XFlush(x11Display);

  window = (NativeWindowType)x11Window;
  return true;
}

bool processEvents() {
  bool bDemoDone = false;

  int i32NumMessages = XPending(x11Display);
  for (int i = 0; i < i32NumMessages; i++) {
    XEvent event;
    XNextEvent(x11Display, &event);

    switch (event.type) {
    // Exit on mouse click
    case ButtonPress:
      bDemoDone = true;
      break;
    default:
      break;
    }
  }
  return bDemoDone;
}
static void exitHandler(int signal) {};

#endif

#ifdef FBDEV

static int tty;
static int o_kdmode;

bool initNativeDisplay() {
  int fd = open("/dev/fb0", O_RDWR);
  if (fd < 0) {
	perror("Error opening fb dev:");
	return false;	
  }
  setenv("EGL_PLATFORM", "fbdev", 0);

  tty = open("/dev/console", O_RDWR);
  ioctl(tty, KDGETMODE, &o_kdmode);
  setExitHandler();
  ioctl(tty, KDSETMODE, KD_GRAPHICS);


  display = (NativeDisplayType)(intptr_t)(fd);
  return true;
}

static void exitHandler(int signal)
{
  ioctl(tty, KDSETMODE, o_kdmode);
  close(tty);
  exit(0);
}

bool initNativeWindow() 
{ 
  window = (NativeWindowType)0; 
  return true; 
}

int getWidth() {
  return 1024;
};
int getHeight() {
  return 768;
};
bool processEvents() {
  return false;
};

#endif

#ifdef RASPI

#include <bcm_host.h>

static int d_width, d_height;

void exitHandler(int signal)
{
}

bool initNativeDisplay() {

  bcm_host_init();
  display = EGL_DEFAULT_DISPLAY;
  return true;
}

bool initNativeWindow() {
  uint32_t width, height;
  int s;

  static EGL_DISPMANX_WINDOW_T nativewindow;

  DISPMANX_ELEMENT_HANDLE_T dispman_element;
  DISPMANX_DISPLAY_HANDLE_T dispman_display;
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  s = graphics_get_display_size(0 /* LCD */, &width, &height);
  assert(s >= 0);

  d_width = width;
  d_height = height;

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = width;
  dst_rect.height = height;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = width << 16;
  src_rect.height = height << 16;

  dispman_display = vc_dispmanx_display_open(0 /* LCD */);
  dispman_update = vc_dispmanx_update_start(0);

  dispman_element = vc_dispmanx_element_add(
      dispman_update, dispman_display, 1 /*layer*/, &dst_rect, 0 /*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0 /*clamp*/,
      (DISPMANX_TRANSFORM_T)0 /*transform*/);

  nativewindow.element = dispman_element;
  nativewindow.width = width;
  nativewindow.height = height;
  vc_dispmanx_update_submit_sync(dispman_update);

  window = (NativeWindowType) &nativewindow;
  return true;
}

int getWidth() {
  return d_width;
};
int getHeight() {
  return d_height;
};
bool processEvents() {
  return false;
};

#endif
