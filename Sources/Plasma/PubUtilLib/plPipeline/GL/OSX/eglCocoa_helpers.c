static int helperGetNextColorMode(GLint *flags, GLint *bpp, GLint *r, GLint *g, GLint *b, GLint *a)
{
    if (((*flags) & kCGLRGB444Bit))       { *flags &= ~kCGLRGB444Bit;       *bpp = 16;  *r = 4;  *g = 4;  *b = 4;  *a = 0;  return 1; }
    if (((*flags) & kCGLARGB4444Bit))     { *flags &= ~kCGLARGB4444Bit;     *bpp = 16;  *r = 4;  *g = 4;  *b = 4;  *a = 4;  return 1; }
    if (((*flags) & kCGLRGB555Bit))       { *flags &= ~kCGLRGB555Bit;       *bpp = 16;  *r = 5;  *g = 5;  *b = 5;  *a = 0;  return 1; }
    if (((*flags) & kCGLARGB1555Bit))     { *flags &= ~kCGLARGB1555Bit;     *bpp = 16;  *r = 5;  *g = 5;  *b = 5;  *a = 1;  return 1; }
    if (((*flags) & kCGLRGB565Bit))       { *flags &= ~kCGLRGB565Bit;       *bpp = 16;  *r = 5;  *g = 6;  *b = 5;  *a = 0;  return 1; }
    if (((*flags) & kCGLRGB888Bit))       { *flags &= ~kCGLRGB888Bit;       *bpp = 32;  *r = 8;  *g = 8;  *b = 8;  *a = 0;  return 1; }
    if (((*flags) & kCGLARGB8888Bit))     { *flags &= ~kCGLARGB8888Bit;     *bpp = 32;  *r = 8;  *g = 8;  *b = 8;  *a = 8;  return 1; }
    if (((*flags) & kCGLRGB101010Bit))    { *flags &= ~kCGLRGB101010Bit;    *bpp = 32;  *r = 10; *g = 10; *b = 10; *a = 0;  return 1; }
    if (((*flags) & kCGLARGB2101010Bit))  { *flags &= ~kCGLARGB2101010Bit;  *bpp = 32;  *r = 10; *g = 10; *b = 10; *a = 2;  return 1; }
    if (((*flags) & kCGLRGB121212Bit))    { *flags &= ~kCGLRGB121212Bit;    *bpp = 48;  *r = 12; *g = 12; *b = 12; *a = 0;  return 1; }
    if (((*flags) & kCGLARGB12121212Bit)) { *flags &= ~kCGLARGB12121212Bit; *bpp = 48;  *r = 12; *g = 12; *b = 12; *a = 12; return 1; }
    if (((*flags) & kCGLRGB161616Bit))    { *flags &= ~kCGLRGB161616Bit;    *bpp = 64;  *r = 16; *g = 16; *b = 16; *a = 0;  return 1; }
    if (((*flags) & kCGLRGBA16161616Bit)) { *flags &= ~kCGLRGBA16161616Bit; *bpp = 64;  *r = 16; *g = 16; *b = 16; *a = 16; return 1; }
    /* skip other formats */
    *flags = 0; return 0;
}

static GLint helperGetNextBitDepth(GLint *flags)
{
    if (((*flags) & kCGL0Bit)) { *flags &= ~kCGL0Bit; return 0; }
    if (((*flags) & kCGL1Bit)) { *flags &= ~kCGL1Bit; return 1; }
    if (((*flags) & kCGL2Bit)) { *flags &= ~kCGL2Bit; return 2; }
    if (((*flags) & kCGL3Bit)) { *flags &= ~kCGL3Bit; return 3; }
    if (((*flags) & kCGL4Bit)) { *flags &= ~kCGL4Bit; return 4; }
    if (((*flags) & kCGL5Bit)) { *flags &= ~kCGL5Bit; return 5; }
    if (((*flags) & kCGL6Bit)) { *flags &= ~kCGL6Bit; return 6; }
    if (((*flags) & kCGL8Bit)) { *flags &= ~kCGL8Bit; return 8; }
    if (((*flags) & kCGL10Bit)) { *flags &= ~kCGL10Bit; return 10; }
    if (((*flags) & kCGL12Bit)) { *flags &= ~kCGL12Bit; return 12; }
    if (((*flags) & kCGL16Bit)) { *flags &= ~kCGL16Bit; return 16; }
    if (((*flags) & kCGL24Bit)) { *flags &= ~kCGL24Bit; return 24; }
    if (((*flags) & kCGL32Bit)) { *flags &= ~kCGL32Bit; return 32; }
    if (((*flags) & kCGL48Bit)) { *flags &= ~kCGL48Bit; return 48; }
    if (((*flags) & kCGL64Bit)) { *flags &= ~kCGL64Bit; return 64; }
    if (((*flags) & kCGL96Bit)) { *flags &= ~kCGL96Bit; return 96; }
    if (((*flags) & kCGL128Bit)) { *flags &= ~kCGL128Bit; return 128; }
    *flags = 0; return 0;
}

#define CALLCGL(f, p, a) \
    do { \
        if ((err = f p) != kCGLNoError) { \
            fprintf(stderr, #f " failed for " #a ": %s\n", CGLErrorString(err)); \
        } \
    } while(0)
#define RINFO(n, v) CALLCGL(CGLDescribeRenderer, (renderer_info, renderer_index, n, &v), v)
#define PFINFO(n, v) CALLCGL(CGLDescribePixelFormat, (c.pf, 0, n, &v), v)

static void helperAddRendererConfigs(OSX_EGLDisplay* display, CGLRendererInfoObj renderer_info, int renderer_index)
{
    CGLError err;

    GLint renderer_id = 0, accelerated = 0;
    RINFO(kCGLRPRendererID, renderer_id);
    RINFO(kCGLRPAccelerated, accelerated);
    GLint window = 0, pbuffer = 1;
    GLint colormode_flags = 0, buffermode_flags = 0;
    GLint depth_flags = 0, stencil_flags = 0;
    RINFO(kCGLRPWindow, window);
    RINFO(kCGLRPColorModes, colormode_flags);
    RINFO(kCGLRPBufferModes, buffermode_flags);
    RINFO(kCGLRPDepthModes, depth_flags);
    RINFO(kCGLRPStencilModes, stencil_flags);

    fprintf(stderr, "0x%x: generating usable pixel formats:\n", renderer_id);

    OSX_EGLConfig c;
    c.renderer_id = renderer_id;
    c.caveat = accelerated ? EGL_NONE : EGL_SLOW_CONFIG;
    c.level = 0;
    c.native_renderable = window ? EGL_TRUE : EGL_FALSE;
    c.transparency_type = EGL_NONE;
    c.transparent_red = 0;
    c.transparent_green = 0;
    c.transparent_blue = 0;

    while (helperGetNextColorMode(&colormode_flags, &c.buffer_bits, &c.red_bits, &c.green_bits, &c.blue_bits, &c.alpha_bits))
    {
        GLint dp = depth_flags;
        do
        {
            c.depth_bits = helperGetNextBitDepth(&dp);
            GLint st = stencil_flags;
            do
            {
                c.stencil_bits = helperGetNextBitDepth(&st);
                CGLPixelFormatAttribute attribs[64], i = 0;
                attribs[i++] = kCGLPFAClosestPolicy;
                if (accelerated) attribs[i++] = kCGLPFAAccelerated;
                if (accelerated) attribs[i++] = kCGLPFANoRecovery;
                if ((buffermode_flags & kCGLDoubleBufferBit)) attribs[i++] = kCGLPFADoubleBuffer;
                attribs[i++] = kCGLPFAColorSize;   attribs[i++] = c.red_bits + c.green_bits + c.blue_bits + c.alpha_bits;
                attribs[i++] = kCGLPFAAlphaSize;   attribs[i++] = c.alpha_bits;
                attribs[i++] = kCGLPFADepthSize;   attribs[i++] = c.depth_bits;
                attribs[i++] = kCGLPFAStencilSize; attribs[i++] = c.stencil_bits;
                attribs[i++] = kCGLPFARendererID;  attribs[i++] = renderer_id;
                attribs[i++] = kCGLPFAOpenGLProfile; attribs[i++] = (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core;
                attribs[i++] = 0;
                GLint npix = 0;

                err = CGLChoosePixelFormat(attribs, &c.pf, &npix);
                if (err == kCGLNoError && npix > 0)
                {
                    GLint cs = -1, a = -1, d = -1, s = -1, glv = 0;
                    PFINFO(kCGLPFAColorSize, cs);
                    PFINFO(kCGLPFAAlphaSize, a);
                    PFINFO(kCGLPFADepthSize, d);
                    PFINFO(kCGLPFAStencilSize, s);
                    PFINFO(kCGLPFAOpenGLProfile, glv);
                    if (cs == c.red_bits + c.green_bits + c.blue_bits + c.alpha_bits &&
                        a == c.alpha_bits && d == c.depth_bits &&
                        s == c.stencil_bits)
                    {
                        CGLRetainPixelFormat(c.pf);
                        c.id = ++display->config_count;
                        c.surface_type = 0;
                        PFINFO(kCGLPFASampleBuffers, c.sample_buffers);
                        PFINFO(kCGLPFASamples, c.samples);

                        fprintf(stderr, "\t0x%02x: %dbpp (%d-%d-%d-%d) depth %-2d stencil %d GL %d\n",
                            c.id, c.buffer_bits, c.red_bits,
                            c.green_bits, c.blue_bits, c.alpha_bits,
                            c.depth_bits, c.stencil_bits, glv);

                        display->cocoa_config = realloc(display->cocoa_config,
                            display->config_count * sizeof(OSX_EGLConfig));
                        display->cocoa_config[display->config_count - 1] = c;
                    }
                    else
                    {
                        // we didn't get what we asked for, trash it
                        CGLReleasePixelFormat(c.pf);
                    }
                }
                else if (err != kCGLNoError)
                {
                    fprintf(stderr, "CGLChoosePixelFormat failed: %s\n", CGLErrorString(err));
                }
            }
            while (st);
        }
        while (dp);
    }
}
#undef RINFO
#undef CALLCGL
