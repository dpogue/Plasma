#define MINIMAL_GL_BUILD

#include "HeadSpin.h"
#include "pnNucleusCreatables.h"
#include "pnMessage/pnMessageCreatable.h"
#include "plDrawable/plDrawableCreatable.h"
#include "plGImage/plGImageCreatable.h"
#include "plGLight/plGLightCreatable.h"
#include "plIntersect/plIntersectCreatable.h"
#include "plMessage/plMessageCreatable.h"
#include "plPipeline/plPipelineCreatable.h"
#include "plScene/plSceneCreatable.h"
#include "plSurface/plSurfaceCreatable.h"


#include <X11/Xlib-xcb.h>
//#include <xcb/xcb.h>
#include <unistd.h>

int main()
{
    Display* display = XOpenDisplay(nullptr);

    /* Open the connection to the X server */
    xcb_connection_t *connection = XGetXCBConnection(display);


    /* Get the first screen */
    const xcb_setup_t      *setup  = xcb_get_setup (connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xcb_screen_t           *screen = iter.data;


    /* Create the window */
    xcb_window_t window = xcb_generate_id (connection);
    xcb_create_window (connection,                    /* Connection          */
                       XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                       window,                        /* window Id           */
                       screen->root,                  /* parent window       */
                       0, 0,                          /* x, y                */
                       800, 600,                      /* width, height       */
                       10,                            /* border_width        */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                       screen->root_visual,           /* visual              */
                       0, NULL );                     /* masks, not used yet */

    /* Map the window on the screen */
    xcb_map_window (connection, window);


    /* Make sure commands are sent before we pause so that the window gets shown */
    xcb_flush (connection);


    hsG3DDeviceRecord dev;
    dev.SetG3DDeviceType(hsG3DDeviceSelector::kDevTypeOpenGL);

    hsG3DDeviceMode mode;
    mode.SetColorDepth(32);
    mode.SetHeight(600);
    mode.SetWidth(800);

    hsG3DDeviceModeRecord devRec(dev, mode);

    plGLPipeline* pipe = new plGLPipeline((hsWindowHndl)display, &devRec);

    pause();    /* hold client until Ctrl-C */

    xcb_disconnect (connection);

    return 0;
}
