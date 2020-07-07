#include <stdio.h>
#include <OpenGL/GL.h>
#include <OpenGL/CGLTypes.h>
#include "eglCocoa.h"

#include "eglCocoa_helpers.c"

static EGLContext current_context = EGL_NO_CONTEXT;


EGLBoolean eglBindAPI(EGLenum api)
{
    if (api != EGL_OPENGL_ES_API && api != EGL_OPENGL_API)
    {
        fprintf(stderr, "WARNING: Unsupported API requested 0x%x!\n", api);
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

EGLenum eglQueryAPI(void)
{
    return EGL_OPENGL_ES_API;
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
    OSX_EGLDisplay* display = malloc(sizeof(OSX_EGLDisplay));

    display->initialized = 0;
    display->config_count = 0;
    display->cocoa_config = NULL;

    if (display_id == EGL_DEFAULT_DISPLAY) {
        display->id = CGMainDisplayID();
    } else {
        display->id = (CGDirectDisplayID)display_id;
    }

    return display;
}


EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    OSX_EGLDisplay* display = dpy;

    if (!display)
    {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (!display->initialized) {
        display->major = 0;
        display->minor = 0;

        CGLGetVersion(&display->major, &display->minor);

        if (!display->cocoa_config) {
            CGLRendererInfoObj renderers;
            GLint nrend = 0;
            CGLError err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(display->id), &renderers, &nrend);
            display->config_count = 0;

            if (err == kCGLNoError) {
                GLint i = 0;

                fprintf(stderr, "available renderers:\n");
                for(; i < nrend; i++)
                {
                    helperAddRendererConfigs(display, renderers, i);
                }
                CGLDestroyRendererInfo(renderers);
            }
            else
            {
                fprintf(stderr, "CGLQueryRendererInfo failed: %s\n", CGLErrorString(err));
            }
        }

        display->initialized = 1;
    }

    if(major) *major = 1;
    if(minor) *minor = 4;

    return EGL_TRUE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
    return EGL_TRUE;
}


EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    OSX_EGLDisplay* display = dpy;
    if (!display)
    {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (!display->initialized)
    {
        fprintf(stderr, "display not initialized!\n");
        //deglSetError(EGL_NOT_INITIALIZED);
        return EGL_FALSE;
    }

    if (!num_config)
    {
        fprintf(stderr, "num_config is NULL!\n");
        //deglSetError(EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }

    int attrib_list_len = attrib_list ? 1 : 0;
    for(EGLint const* p = attrib_list; p && *p != EGL_NONE; p+=2, attrib_list_len+=2);

    fprintf(stderr, "%d attributes..\n", (attrib_list_len - 1) / 2);

#if 0
    for(int i = 0; i < attrib_list_len - 1; i += 2)
    {
        const char *p = degl_attrib_name(attrib_list[i]);
        if(p)
        {
            fprintf(stderr, "\t%s = 0x%x\n", p, attrib_list[i + 1]);
        }
        else
        {
            fprintf(stderr, "\tunknown(0x%x) = 0x%x\n", attrib_list[i], attrib_list[i + 1]);
        }
    }
#endif

    OSX_EGLConfig crit =
    {
        .buffer_bits = 0,
        .red_bits = 0,
        .green_bits = 0,
        .blue_bits = 0,
        .alpha_bits = 0,
        .caveat = EGL_DONT_CARE,
        .id = EGL_DONT_CARE,
        .depth_bits = 0,
        .level = 0,
        .native_renderable = EGL_DONT_CARE,
        .sample_buffers = 0,
        .samples = 0,
        .stencil_bits = 0,
        .surface_type = 0,
        .transparency_type = EGL_NONE,
        .transparent_red = EGL_DONT_CARE,
        .transparent_green = EGL_DONT_CARE,
        .transparent_blue = EGL_DONT_CARE,
    };
    int pixmap_bits = EGL_DONT_CARE;
    int i, j;
    for(i = 0; i < attrib_list_len - 1; i += 2)
    {
        int value = attrib_list[i + 1];
        switch(attrib_list[i])
        {
            case EGL_BUFFER_SIZE: crit.buffer_bits = value; break;
            case EGL_RED_SIZE: crit.red_bits = value; break;
            case EGL_GREEN_SIZE: crit.green_bits = value; break;
            case EGL_BLUE_SIZE: crit.blue_bits = value; break;
            case EGL_LUMINANCE_SIZE: /* ignored */ break;
            case EGL_ALPHA_SIZE: crit.alpha_bits = value; break;
            case EGL_ALPHA_MASK_SIZE: /* ignored */ break;
            case EGL_BIND_TO_TEXTURE_RGB: /* ignored */ break;
            case EGL_BIND_TO_TEXTURE_RGBA: /* ignored */ break;
            case EGL_COLOR_BUFFER_TYPE:
                // we have RGB buffers available only
                if(value != EGL_DONT_CARE && value != EGL_RGB_BUFFER)
                {
                    *num_config = 0;
                    return EGL_TRUE;
                }
                break;
            case EGL_CONFIG_CAVEAT: crit.caveat = value; break;
            case EGL_CONFIG_ID: crit.id = value; break;
            case EGL_CONFORMANT: /* ignored */ break;
            case EGL_DEPTH_SIZE: crit.depth_bits = value; break;
            case EGL_LEVEL: crit.level = value; break;
            case EGL_MATCH_NATIVE_PIXMAP: break;
            case EGL_MAX_PBUFFER_WIDTH: /* ignored */ break;
            case EGL_MAX_PBUFFER_HEIGHT: /* ignored */ break;
            case EGL_MAX_PBUFFER_PIXELS: /* ignored */ break;
            case EGL_MAX_SWAP_INTERVAL: /* ignored */ break;
            case EGL_MIN_SWAP_INTERVAL: /* ignored */ break;
            case EGL_NATIVE_RENDERABLE: crit.native_renderable = value; break;
            case EGL_NATIVE_VISUAL_ID: /* ignored */ break;
            case EGL_NATIVE_VISUAL_TYPE:
                /* TODO */
                fprintf(stderr, "EGL_NATIVE_VISUAL_TYPE attribute ignored!\n");
                break;
            case EGL_RENDERABLE_TYPE: /* ignored */ break;
            case EGL_SAMPLE_BUFFERS: crit.sample_buffers = value; break;
            case EGL_SAMPLES: crit.samples = value; break;
            case EGL_STENCIL_SIZE: crit.stencil_bits = value; break;
            case EGL_SURFACE_TYPE: crit.surface_type = value; break;
            case EGL_TRANSPARENT_TYPE: crit.transparency_type = value; break;
            case EGL_TRANSPARENT_RED_VALUE: crit.transparent_red = value; break;
            case EGL_TRANSPARENT_GREEN_VALUE: crit.transparent_green = value; break;
            case EGL_TRANSPARENT_BLUE_VALUE: crit.transparent_blue = value; break;
            default:
                fprintf(stderr, "unknown attribute 0x%x!\n", attrib_list[i]);
                //deglSetError(EGL_BAD_ATTRIBUTE);
                return EGL_FALSE;
        }
    }
    for(i = 0, *num_config = 0; i < display->config_count; i++)
    {
        OSX_EGLConfig *c = &display->cocoa_config[i];

        if((crit.red_bits != EGL_DONT_CARE && c->red_bits < crit.red_bits) ||
            (crit.green_bits != EGL_DONT_CARE && c->green_bits < crit.green_bits) ||
            (crit.blue_bits != EGL_DONT_CARE && c->blue_bits < crit.blue_bits) ||
            (crit.alpha_bits != EGL_DONT_CARE && c->alpha_bits < crit.alpha_bits) ||
            (crit.buffer_bits != EGL_DONT_CARE && c->buffer_bits < crit.buffer_bits) ||
            (crit.depth_bits != EGL_DONT_CARE && c->depth_bits < crit.depth_bits) ||
            (crit.stencil_bits != EGL_DONT_CARE && c->stencil_bits < crit.stencil_bits) ||
            //!(c->surface_type & crit.surface_type) ||
            (crit.id != EGL_DONT_CARE && c->id != crit.id) ||
            (crit.caveat != EGL_DONT_CARE && c->caveat != crit.caveat) ||
            (crit.transparency_type != EGL_DONT_CARE && crit.transparency_type != c->transparency_type) ||
            (crit.transparent_red != EGL_DONT_CARE && c->transparent_red != crit.transparent_red) ||
            (crit.transparent_green != EGL_DONT_CARE && c->transparent_green != crit.transparent_green) ||
            (crit.transparent_blue != EGL_DONT_CARE && c->transparent_blue != crit.transparent_blue) ||
            (crit.level != c->level) ||
            (crit.native_renderable != EGL_DONT_CARE && c->native_renderable != crit.native_renderable) ||
            (crit.sample_buffers != EGL_DONT_CARE && c->sample_buffers < crit.sample_buffers) ||
            (crit.samples != EGL_DONT_CARE && c->samples < crit.samples) ||
            (pixmap_bits != EGL_DONT_CARE && (c->buffer_bits != pixmap_bits || !(c->surface_type & EGL_PIXMAP_BIT))))
        {
            //fprintf(stderr, "config 0x%x does not meet minimum requirements\n", c->id);
            continue;
        }
        if(configs)
        {
            int color_bits = ((crit.red_bits != EGL_DONT_CARE && crit.red_bits) ? c->red_bits : 0) +
                ((crit.green_bits != EGL_DONT_CARE && crit.green_bits) ? c->green_bits : 0) +
                ((crit.blue_bits != EGL_DONT_CARE && crit.blue_bits) ? c->blue_bits : 0) +
                ((crit.alpha_bits != EGL_DONT_CARE && crit.alpha_bits) ? c->alpha_bits : 0);
            for(j = 0; j < *num_config; j++)
            {
                OSX_EGLConfig *refc = configs[j];
                if(crit.caveat == EGL_DONT_CARE && c->caveat != refc->caveat)
                {
                    if(c->caveat == EGL_NONE) break;
                    if(refc->caveat == EGL_NONE) continue;
                    if(c->caveat == EGL_SLOW_CONFIG) break;
                    if(refc->caveat == EGL_SLOW_CONFIG) continue;
                }
                EGLint ref = ((crit.red_bits != EGL_DONT_CARE && crit.red_bits) ? refc->red_bits : 0) +
                    ((crit.green_bits != EGL_DONT_CARE && crit.green_bits) ? refc->green_bits : 0) +
                    ((crit.blue_bits != EGL_DONT_CARE && crit.blue_bits) ? refc->blue_bits : 0) +
                    ((crit.alpha_bits != EGL_DONT_CARE && crit.alpha_bits) ? refc->alpha_bits : 0);
                if(color_bits > ref) break;
                if(color_bits < ref) continue;
                if(c->buffer_bits < refc->buffer_bits) break;
                if(c->buffer_bits > refc->buffer_bits) continue;
                if(c->sample_buffers < refc->sample_buffers) break;
                if(c->sample_buffers > refc->sample_buffers) continue;
                if(c->samples < refc->samples) break;
                if(c->samples > refc->samples) continue;
                if(c->depth_bits < refc->depth_bits) break;
                if(c->depth_bits > refc->depth_bits) continue;
                if(c->stencil_bits < refc->stencil_bits) break;
                if(c->stencil_bits > refc->stencil_bits) continue;
                if(c->id < refc->id) break;
                if(c->id > refc->id) continue;
            }
            if(j < config_size)
            {
                if(j < (*num_config) - 1)
                {
                    memcpy(&configs[j + 1], &configs[j], (*num_config - j) * sizeof(EGLConfig));
                }
                if(*num_config < config_size)
                {
                    (*num_config)++;
                }
                configs[j] = c;
            }
        }
        else
        {
            (*num_config)++;
        }
    }
    return EGL_TRUE;
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config_, EGLNativeWindowType win, const EGLint *attrib_list)
{
    OSX_EGLDisplay* display = dpy;
    OSX_EGLConfig* config = config_;
    OSX_EGLSurface *surface = NULL;

    if (!display)
    {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_NO_SURFACE;
    }

    if (!display->initialized)
    {
        fprintf(stderr, "display not initialized!\n");
        //deglSetError(EGL_NOT_INITIALIZED);
        return EGL_NO_SURFACE;
    }

    if (!config)
    {
        fprintf(stderr, "bad config!\n");
        //deglSetError(EGL_BAD_CONFIG);
        return EGL_NO_SURFACE;
    }

    if (!win)
    {
        fprintf(stderr, "bad native window!\n");
        //deglSetError(EGL_BAD_NATIVE_WINDOW);
        return EGL_NO_SURFACE;
    }

    surface = malloc(sizeof(*surface));
    if (!surface)
    {
        fprintf(stderr, "malloc failed!\n");
        //deglSetError(EGL_BAD_ALLOC);
        return EGL_NO_SURFACE;
    }

    surface->bound = 0;
    surface->destroy = 0;
    surface->type = OSX_EGLSurface_window;
    surface->config = config;

    surface->depth = config->buffer_bits;
    surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
    surface->nsview = CreateView(surface, win, &surface->width, &surface->height);

    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    free(surface);

    return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config_, EGLContext share_, const EGLint *attrib_list)
{
    OSX_EGLDisplay* display = dpy;
    OSX_EGLConfig* config = config_;
    OSX_EGLContext* share_context = (OSX_EGLContext*)share_;
    OSX_EGLContext* context = NULL;

    if (!display)
    {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_NO_CONTEXT;
    }

    if (!display->initialized)
    {
        fprintf(stderr, "display not initialized!\n");
        //deglSetError(EGL_NOT_INITIALIZED);
        return EGL_NO_CONTEXT;
    }

    if (!config)
    {
        fprintf(stderr, "bad config!\n");
        //deglSetError(EGL_BAD_CONFIG);
        return EGL_NO_CONTEXT;
    }

    context = malloc(sizeof(*context));
    context->display    = dpy;
    context->config     = config;
    context->version    = 1;
    context->clientlib  = 0;
    context->clientdata = 0;
    context->bound      = 0;
    context->destroy    = 0;
    context->draw       = 0;
    context->read       = 0;

    // Figure the context version..
    if (attrib_list) {
        unsigned i = 0;
        for (; attrib_list[i] != EGL_NONE; i += 2) {
            if (attrib_list[i] == EGL_CONTEXT_CLIENT_VERSION) {
                context->version = attrib_list[i + 1];
            }
        }
    }
    fprintf(stderr, "Context version %d creation requested.\n", context->version);

    CGLError err = CGLCreateContext(((OSX_EGLConfig*)config_)->pf, share_context ? share_context->ctx : NULL, &context->ctx);
    if(err != kCGLNoError)
    {
        fprintf(stderr, "CGLCreateContext failed: %s\n", CGLErrorString(err));
        free(context);
        //deglSetError(EGL_BAD_ALLOC);
        return EGL_NO_CONTEXT;
    }

    CGLRetainContext(context->ctx);
    context->nsctx = NULL;
    return context;
}


EGLContext eglGetCurrentContext(void)
{
    return current_context;
}


EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    OSX_EGLDisplay* display = dpy;
    OSX_EGLSurface* draw_surface = draw;
    OSX_EGLSurface* read_surface = read;
    OSX_EGLContext* context = ctx;

    if (!display) {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (!display->initialized && (ctx != EGL_NO_CONTEXT || draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE)) {
        fprintf(stderr, "display not initialized!\n");
        //deglSetError(EGL_NOT_INITIALIZED);
        return EGL_FALSE;
    }

    if ((ctx == EGL_NO_CONTEXT && draw != EGL_NO_SURFACE && read != EGL_NO_SURFACE) || (ctx != EGL_NO_CONTEXT && draw == EGL_NO_SURFACE && read == EGL_NO_SURFACE)) {
        fprintf(stderr, "bad match!\n");
        //deglSetError(EGL_BAD_MATCH);
        return EGL_FALSE;
    }

    OSX_EGLContext* old_context = (OSX_EGLContext*)eglGetCurrentContext();
    fprintf(stderr, "draw=%p, read=%p, context=%p (current context %p)\n", draw, read, ctx, old_context);


    if (draw_surface != read_surface) {
        fprintf(stderr, "WARNING: Draw and read surfaces are different!\n");
    }

    CGLError err = CGLSetCurrentContext(context ? context->ctx : NULL);
    if (err != kCGLNoError) {
        fprintf(stderr, "CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
        //deglSetError(EGL_BAD_ALLOC);
        return EGL_FALSE;
    }

    if (context && draw_surface) {
        GLint screen = 0;
        switch (draw_surface->type)
        {
            case OSX_EGLSurface_window:
                if (context->nsctx == NULL) {
                    context->nsctx = CreateContext(context->ctx);
                }
                SetView(context->nsctx, draw_surface->nsview);
                break;
            case OSX_EGLSurface_pbuffer:
                err = CGLGetVirtualScreen(context->ctx, &screen);
                if (err == kCGLNoError) {
                    err = CGLSetPBuffer(context->ctx, draw_surface->pbuffer, 0, 0, screen);
                }
                break;
            default:
                fprintf(stderr, "unsupported cocoa surface type %d\n", draw_surface->type);
                break;
        }

        if (err != kCGLNoError) {
            fprintf(stderr, "CGL drawable activation: %s\n", CGLErrorString(err));
            //deglSetError(EGL_BAD_ALLOC);
            return EGL_FALSE;
        }
    }

    if (ctx != EGL_NO_CONTEXT)
    {
        //deglRefContext(context);
        //deglRefSurface(draw_surface);
        //deglRefSurface(read_surface);

        //deglActivateClientAPI(context);
    }

    if (old_context != EGL_NO_CONTEXT && old_context)
    {
        //deglUnrefSurface(dpy, &old_context->draw);
        //deglUnrefSurface(dpy, &old_context->read);
        //deglUnrefContext(dpy, &old_context);
    }

    if (ctx != EGL_NO_CONTEXT)
    {
        context->draw = draw_surface;
        context->read = read_surface;
    }

    fprintf(stderr, "Setting %p as current.\n", ctx);
    current_context = ctx;
    return EGL_TRUE;
}


EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface_)
{
    OSX_EGLDisplay* display = dpy;
    OSX_EGLSurface* surface = surface_;
    OSX_EGLContext* context = (OSX_EGLContext*)eglGetCurrentContext();

    if (!display) {
        fprintf(stderr, "bad display!\n");
        //deglSetError(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (!display->initialized) {
        fprintf(stderr, "display not initialized!\n");
        //deglSetError(EGL_NOT_INITIALIZED);
        return EGL_FALSE;
    }

    if (!context) {
        fprintf(stderr, "bad context!\n");
        //deglSetError(EGL_BAD_CONTEXT);
        return EGL_FALSE;
    }

    if (!surface) {
        fprintf(stderr, "bad surface!\n");
        //deglSetError(EGL_BAD_SURFACE);
        return EGL_FALSE;
    }

    glFlush();

    CGLError err = CGLFlushDrawable(context->ctx);
    if(err != kCGLNoError) {
        fprintf(stderr, "CGLFlushDrawable failed: %s\n", CGLErrorString(err));
    }

    return EGL_TRUE;
}
