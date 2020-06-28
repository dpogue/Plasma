/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/

#include "plGLClient/plGLClient.h"
#include "plGLClient/plClientLoader.h"

#include "plResMgr/plResManager.h"
#include "plClientResMgr/plClientResMgr.h"

#include "plCmdParser.h"

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>
#include <unistd.h>

enum
{
    kArgSkipLoginDialog,
    kArgServerIni,
    kArgLocalData,
    kArgSkipPreload,
    kArgPlayerId,
    kArgStartUpAgeName,
};

static const plCmdArgDef s_cmdLineArgs[] = {
    //{ kCmdArgFlagged  | kCmdTypeBool,       "SkipLoginDialog", kArgSkipLoginDialog },
    //{ kCmdArgFlagged  | kCmdTypeString,     "ServerIni",       kArgServerIni },
    { kCmdArgFlagged  | kCmdTypeBool,       "LocalData",       kArgLocalData   },
    //{ kCmdArgFlagged  | kCmdTypeBool,       "SkipPreload",     kArgSkipPreload },
    //{ kCmdArgFlagged  | kCmdTypeInt,        "PlayerId",        kArgPlayerId },
    { kCmdArgFlagged  | kCmdTypeString,     "Age",             kArgStartUpAgeName },
};

extern bool gDataServerLocal;

plClientLoader gClient;
xcb_connection_t* gXConn;


void PumpMessageQueueProc()
{
    xcb_generic_event_t* event = xcb_poll_for_event(gXConn);
    if (event && ((event->response_type & ~0x80) == XCB_KEY_PRESS))
    {
        xcb_key_press_event_t* kbe = reinterpret_cast<xcb_key_press_event_t*>(event);

        if (kbe->detail == 24) {
            gClient->SetDone(true);
        }
    }
}


int main(int argc, char** argv)
{
    std::vector<ST::string> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++)
    {
        args.push_back(ST::string::from_utf8(argv[i]));
    }

    plCmdParser cmdParser(s_cmdLineArgs, std::size(s_cmdLineArgs));
    cmdParser.Parse(args);

    if (!XInitThreads())
    {
        hsStatusMessage("Failed to initialize X11 in thread-safe mode");
        return 1;
    }

    /* Open the connection to the X server */
    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
    gXConn = connection;


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

    const char* title = "plGLClient";
    xcb_change_property(connection,
                        XCB_PROP_MODE_REPLACE,
                        window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(title),
                        title);

    /* Map the window on the screen */
    xcb_map_window(connection, window);

    /* Make sure commands are sent before we pause so that the window gets shown */
    xcb_flush(connection);

    Display* display = XOpenDisplay(nullptr);


    gClient.SetClientWindow((hsWindowHndl)(uintptr_t)window);
    gClient.SetClientDisplay((hsWindowHndl)display);
    gClient.Init();

    // We should quite frankly be done initing the client by now. But, if not, spawn the good old
    // "Starting URU, please wait..." dialog (not so yay)
    if (!gClient.IsInited())
    {
        gClient.Wait();
    }

#if 1
    // X11 & EGL & OpenGL aren't playing nice with threads. This code should
    // be run in plClientLoader::Run but has to be here for now.
    gClient->SetWindowHandle((hsWindowHndl)(uintptr_t)window);

    if (gClient->InitPipeline((hsWindowHndl)display) || !gClient->StartInit())
    {
        gClient->SetDone(true);
    }
#endif

    if (cmdParser.IsSpecified(kArgLocalData))
    {
        gDataServerLocal = true;
    }

    if (cmdParser.IsSpecified(kArgStartUpAgeName))
    {
        gDataServerLocal = true;
        gClient->SetQuitIntro(true);
    }

    // Main loop
    if (gClient && !gClient->GetDone())
    {
        gClient->SetMessagePumpProc(PumpMessageQueueProc);
        gClient.Start();

        do
        {
            gClient->MainLoop();

            if (gClient->GetDone()) {
                break;
            }

            PumpMessageQueueProc();

        } while (true);
    }


    gClient.ShutdownEnd();

    xcb_disconnect(connection);

    return 0;
}
