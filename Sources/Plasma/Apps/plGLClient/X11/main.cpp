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

#include "plProduct.h"
#include "plGLClient/plGLClient.h"
#include "plGLClient/plClientLoader.h"

#include "pcSmallRect.h"
#include "plInputCore/plInputManager.h"
#include "plMessage/plInputEventMsg.h"

#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/xproto.h>
#include <X11/Xlib-xcb.h>
#include <unistd.h>

plClientLoader gClient;
xcb_connection_t* gXConn;
bool gHasXFixes = false;
pcSmallRect gWindowSize;

const unsigned char KEYCODE_LINUX_TO_HID[256] = {
    0,41,30,31,32,33,34,35,36,37,38,39,45,46,42,43,20,26,8,21,23,28,24,12,18,19,
    47,48,158,224,4,22,7,9,10,11,13,14,15,51,52,53,225,49,29,27,6,25,5,17,16,54,
    55,56,229,85,226,44,57,58,59,60,61,62,63,64,65,66,67,83,71,95,96,97,86,92,
    93,94,87,89,90,91,98,99,0,0,100,68,69,0,0,0,0,0,0,0,88,228,84,154,230,0,74,
    82,75,80,79,77,81,78,73,76,0,0,0,0,0,103,0,72,0,0,0,0,0,227,231,0,0,0,0,0,0,
    0,0,0,0,0,0,118,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,104,105,106,107,108,109,110,111,112,113,114,115,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#include "pfConsoleCore/pfConsoleEngine.h"
PF_CONSOLE_LINK_ALL()

void PumpMessageQueueProc()
{
    xcb_generic_event_t* event;

    while (event = xcb_poll_for_event(gXConn)) {
        switch (event->response_type & ~0x80)
        {
        case XCB_CONFIGURE_NOTIFY: // Window resize
            {
                xcb_configure_notify_event_t* cne = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                gWindowSize.Set(cne->x, cne->y, cne->width, cne->height);
            }
            break;

        case XCB_KEY_PRESS:     // Keyboard key press
        case XCB_KEY_RELEASE:   // Keyboard key release
            {
                xcb_key_press_event_t* kbe = reinterpret_cast<xcb_key_press_event_t*>(event);

                bool down = (kbe->response_type & ~0x80) == XCB_KEY_PRESS;

                /* X11 offsets Linux keycodes by 8 */
                uint32_t keycode = kbe->detail - 8;
                plKeyDef key = KEY_UNMAPPED;
                if (keycode < 256)
                {
                    key = (plKeyDef)KEYCODE_LINUX_TO_HID[keycode];
                }

                #ifdef MINIMAL_GL_BUILD
                if (key == KEY_Q) { // Quit when Q is hit
                    gClient->SetDone(true);
                    break;
                }
                #endif

                if (down)
                {
                    gClient->SetQuitIntro(true);
                }

                gClient->GetInputManager()->HandleKeyEvent(key, down, false);
            }
            break;

        case XCB_ENTER_NOTIFY: // Mouse over windows
            {
                if (gHasXFixes)
                {
                    xcb_enter_notify_event_t* ene = reinterpret_cast<xcb_enter_notify_event_t*>(event);
                    xcb_xfixes_hide_cursor(gXConn, ene->root);
                    xcb_flush(gXConn);
                }
            }
            break;

        case XCB_LEAVE_NOTIFY: // Mouse off windows
            {
                if (gHasXFixes)
                {
                    xcb_leave_notify_event_t* lne = reinterpret_cast<xcb_leave_notify_event_t*>(event);
                    xcb_xfixes_show_cursor(gXConn, lne->root);
                    xcb_flush(gXConn);
                }
            }
            break;

        case XCB_MOTION_NOTIFY: // Mouse Movement
            {
                xcb_motion_notify_event_t* me = reinterpret_cast<xcb_motion_notify_event_t*>(event);

                plIMouseXEventMsg* pXMsg = new plIMouseXEventMsg;
                plIMouseYEventMsg* pYMsg = new plIMouseYEventMsg;

                pXMsg->fWx = me->event_x;
                pXMsg->fX = (float)me->event_x / (float)gWindowSize.fWidth;

                pYMsg->fWy = me->event_y;
                pYMsg->fY = (float)me->event_y / (float)gWindowSize.fHeight;

                gClient->GetInputManager()->MsgReceive(pXMsg);
                gClient->GetInputManager()->MsgReceive(pYMsg);

                delete(pXMsg);
                delete(pYMsg);
            }
            break;

        case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t* bpe = reinterpret_cast<xcb_button_press_event_t*>(event);

                /* Handle scroll wheel */
                if (bpe->detail == XCB_BUTTON_INDEX_4 || bpe->detail == XCB_BUTTON_INDEX_5)
                {
                /*
                case XCB_BUTTON_INDEX_4:
                    pMsg->fButton |= kWheelPos;
                    pMsg->SetWheelDelta(120.0f);
                    break;
                case XCB_BUTTON_INDEX_5:
                    pMsg->fButton |= kWheelNeg;
                    pMsg->SetWheelDelta(-120.0f);
                    break;
                */
                    break;
                }

                plIMouseXEventMsg* pXMsg = new plIMouseXEventMsg;
                plIMouseYEventMsg* pYMsg = new plIMouseYEventMsg;
                plIMouseBEventMsg* pBMsg = new plIMouseBEventMsg;

                pXMsg->fWx = bpe->event_x;
                pXMsg->fX = (float)bpe->event_x / (float)gWindowSize.fWidth;

                pYMsg->fWy = bpe->event_y;
                pYMsg->fY = (float)bpe->event_y / (float)gWindowSize.fHeight;

                switch (bpe->detail) {
                case XCB_BUTTON_INDEX_1:
                    pBMsg->fButton |= kLeftButtonDown;
                    break;
                case XCB_BUTTON_INDEX_2:
                    pBMsg->fButton |= kMiddleButtonDown;
                    break;
                case XCB_BUTTON_INDEX_3:
                    pBMsg->fButton |= kRightButtonDown;
                    break;
                default:
                    break;
                }

                gClient->GetInputManager()->MsgReceive(pXMsg);
                gClient->GetInputManager()->MsgReceive(pYMsg);
                gClient->GetInputManager()->MsgReceive(pBMsg);

                delete(pXMsg);
                delete(pYMsg);
                delete(pBMsg);
            }
            break;

        case XCB_BUTTON_RELEASE:
            {
                xcb_button_release_event_t* bre = reinterpret_cast<xcb_button_release_event_t*>(event);

                plIMouseXEventMsg* pXMsg = new plIMouseXEventMsg;
                plIMouseYEventMsg* pYMsg = new plIMouseYEventMsg;
                plIMouseBEventMsg* pBMsg = new plIMouseBEventMsg;

                pXMsg->fWx = bre->event_x;
                pXMsg->fX = (float)bre->event_x / (float)gWindowSize.fWidth;

                pYMsg->fWy = bre->event_y;
                pYMsg->fY = (float)bre->event_y / (float)gWindowSize.fHeight;

                switch (bre->detail) {
                case XCB_BUTTON_INDEX_1:
                    pBMsg->fButton |= kLeftButtonUp;
                    break;
                case XCB_BUTTON_INDEX_2:
                    pBMsg->fButton |= kMiddleButtonUp;
                    break;
                case XCB_BUTTON_INDEX_3:
                    pBMsg->fButton |= kRightButtonUp;
                    break;
                default:
                    break;
                }

                gClient->GetInputManager()->MsgReceive(pXMsg);
                gClient->GetInputManager()->MsgReceive(pYMsg);
                gClient->GetInputManager()->MsgReceive(pBMsg);

                delete(pXMsg);
                delete(pYMsg);
                delete(pBMsg);
            }
            break;

        default:
            break;
        }

        free(event);
    }
}


int main(int argc, const char** argv)
{
    PF_CONSOLE_INITIALIZE(Audio)

    if (!XInitThreads())
    {
        hsStatusMessage("Failed to initialize X11 in thread-safe mode");
        return 1;
    }

    /* Open the connection to the X server */
    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
    gXConn = connection;


    /* Get the first screen */
    const xcb_setup_t* setup = xcb_get_setup(gXConn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    /* Check for XFixes support for hiding the cursor */
    const xcb_query_extension_reply_t* qe_reply = xcb_get_extension_data(gXConn, &xcb_xfixes_id);
    if (qe_reply && qe_reply->present)
    {
        /* We *must* negotiate the XFixes version with the server */
        xcb_xfixes_query_version_cookie_t qv_cookie = xcb_xfixes_query_version(gXConn, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
        xcb_xfixes_query_version_reply_t* qv_reply = xcb_xfixes_query_version_reply(gXConn, qv_cookie, nullptr);

#ifndef HS_DEBUGGING // Don't hide the cursor when debugging
        gHasXFixes = qv_reply->major_version >= 4;
#endif

        free(qv_reply);
    }


    const uint32_t event_mask = XCB_EVENT_MASK_EXPOSURE
                              | XCB_EVENT_MASK_KEY_PRESS
                              | XCB_EVENT_MASK_KEY_RELEASE
                              | XCB_EVENT_MASK_POINTER_MOTION
                              | XCB_EVENT_MASK_BUTTON_PRESS
                              | XCB_EVENT_MASK_BUTTON_RELEASE
                              | XCB_EVENT_MASK_ENTER_WINDOW
                              | XCB_EVENT_MASK_LEAVE_WINDOW
                              | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    /* Create the window */
    xcb_window_t window = xcb_generate_id(gXConn);
    xcb_create_window(gXConn,                        /* Connection          */
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

    const char* title = plProduct::LongName().c_str();
    xcb_change_property(gXConn,
                        XCB_PROP_MODE_REPLACE,
                        window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(title),
                        title);

    /* Map the window on the screen */
    xcb_map_window(gXConn, window);

    /* Make sure commands are sent before we pause so that the window gets shown */
    xcb_flush(gXConn);

    Display* display = XOpenDisplay(nullptr);

    gWindowSize.Set(0, 0, 800, 600);

    gClient.SetClientWindow((hsWindowHndl)(uintptr_t)window);
    gClient.SetClientDisplay((hsWindowHndl)display);
    gClient.Init(argc, argv);

    // We should quite frankly be done initing the client by now. But, if not, spawn the good old
    // "Starting URU, please wait..." dialog (not so yay)
    if (!gClient.IsInited())
    {
        gClient.Wait();
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

    xcb_disconnect(gXConn);

    return 0;
}
