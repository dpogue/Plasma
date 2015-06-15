/* Copyright (C) 2011  Nokia Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <EGL/egl.h>

#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#include <OpenGL/OpenGL.h>

typedef struct OSX_EGLConfig
{
    EGLint buffer_bits;
    EGLint red_bits;
    EGLint green_bits;
    EGLint blue_bits;
    // luminance
    EGLint alpha_bits;
    // alpha mask
    // bind to texture rgb
    // bind to texture rgba
    // color buffer type
    EGLint caveat;
    EGLint id;
    // conformant
    EGLint depth_bits;
    EGLint level;
    // max swap interval
    // min swap interval
    EGLint native_renderable;
    // native visual type
    // renderable type
    EGLint sample_buffers;
    EGLint samples;
    EGLint stencil_bits;
    EGLint surface_type;
    EGLint transparency_type;
    EGLint transparent_red;
    EGLint transparent_green;
    EGLint transparent_blue;

    GLint renderer_id;
    CGLPixelFormatObj pf;
} OSX_EGLConfig;


typedef struct OSX_EGLDisplay
{
    int initialized;
    int major, minor; // backend version info
    int config_count;

    CGDirectDisplayID id;
    OSX_EGLConfig* cocoa_config;
} OSX_EGLDisplay;


typedef struct DEGLSurface
{
    int bound;
    int destroy;
    enum
    {
        OSX_EGLSurface_invalid = 0,
        OSX_EGLSurface_window,
        OSX_EGLSurface_pixmap,
        OSX_EGLSurface_pbuffer,
        OSX_EGLSurface_offscreen
    } type;
    unsigned width;  /* width in pixels */
    unsigned height; /* height in pixels */
    unsigned depth;  /* color bit depth */
    unsigned bpp;    /* bytes per pixel */
    OSX_EGLConfig* config;

    CGLPBufferObj pbuffer;
    void* nsview; // NSView*
} OSX_EGLSurface;


typedef struct OSX_EGLContext
{
    EGLDisplay display;
    EGLConfig config;
    int version;
    void* clientlib;
    void* clientdata;
    //GLESFuncs* glesfuncs;
    int bound;
    int destroy;
    OSX_EGLSurface* draw;
    OSX_EGLSurface* read;
    pthread_mutex_t lock;

    CGLContextObj ctx;
    void* nsctx; // NSOpenGLContext*
    int pbuffer_mode;
} OSX_EGLContext;


#ifdef __cplusplus
extern "C" {
#endif

void* CreateContext(CGLContextObj ctx);
void DestroyContext(void* nsctx);
void* CreateView(EGLSurface s, void* nsview, unsigned* width, unsigned* height);
void DestroyView(void* cview);
void SetView(void* nsctx, void* cview);

#ifdef __cplusplus
}
#endif
