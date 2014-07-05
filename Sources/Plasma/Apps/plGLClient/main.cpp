#include "plGLClient.h"

#include "plResMgr/plResManager.h"

#include <xcb/xcb.h>
#include <unistd.h>

int main()
{
    /* Open the connection to the X server */
    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);


    /* Get the first screen */
    const xcb_setup_t      *setup  = xcb_get_setup(connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator(setup);
    xcb_screen_t           *screen = iter.data;


    const uint32_t event_mask = XCB_EVENT_MASK_KEY_PRESS;

    /* Create the window */
    xcb_window_t window = xcb_generate_id(connection);
    xcb_create_window(connection,                    /* Connection          */
                      XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                      window,                        /* window Id           */
                      screen->root,                  /* parent window       */
                      0, 0,                          /* x, y                */
                      800, 600,                      /* width, height       */
                      10,                            /* border_width        */
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                      screen->root_visual,           /* visual              */
                      XCB_CW_EVENT_MASK,             /* masks               */
                      &event_mask);                  /* masks               */

    /* Map the window on the screen */
    xcb_map_window(connection, window);


    /* Make sure commands are sent before we pause so that the window gets shown */
    xcb_flush(connection);


    plResManager *resMgr = new plResManager();
    resMgr->SetDataPath("dat");
    hsgResMgr::Init(resMgr);

    plGLClient* gClient = new plGLClient();
    if (gClient == nullptr)
    {
        return 1;
    }

    gClient->SetWindowHandle((hsWindowHndl)window);

    if (gClient->InitPipeline())
    {
        return 1;
    }

    if (!gClient->StartInit())
    {
        return 1;
    }


    do
    {
        gClient->MainLoop();

        if (gClient->GetDone()) {
            break;
        }

        xcb_generic_event_t* event = xcb_poll_for_event(connection);
        if (event && ((event->response_type & ~0x80) == XCB_KEY_PRESS))
        {
            gClient->SetDone(true);
        }
    } while (true);


    if (gClient)
    {
        gClient->Shutdown();
        gClient = nullptr;
    }

    hsAssert(hsgResMgr::ResMgr()->RefCnt()==1, "resMgr has too many refs, expect mem leaks");
    hsgResMgr::Shutdown(); // deletes fResMgr


    xcb_disconnect(connection);

    return 0;
}
