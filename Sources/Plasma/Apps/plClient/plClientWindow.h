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

#include "HeadSpin.h"
#include "pcSmallRect.h"

class plClientWindow
{
protected:
    pcSmallRect fWindowSize;

public:
    plClientWindow() : fWindowSize(0, 0, 800, 600) { }
    virtual ~plClientWindow() = default;

    /**
     * Does any necessary pre-initialization work to set up/configure the
     * environment before trying to create a window.
     *
     * Returns false on failure, which will stop the client launch.
     */
    virtual bool PreInit() { return true; }

    /**
     * Creates the client window, but does not display it on the screen.
     *
     * Returns false on failure, which will stop the client launch.
     */
    virtual bool CreateClientWindow() = 0;

    /**
     * Makes the client window visible on the screen.
     */
    virtual void ShowClientWindow() = 0;

    /**
     * Resizes the client window to the specified dimensions, along with making
     * it fullscreen or not.
     */
    virtual void ResizeClientWindow(uint16_t width, uint16_t height, bool windowed) = 0;

    /**
     * Process any OS/Window events that are in the queue.
     *
     * This essentially runs the event loop until there are no more messages,
     * then returns to allow the plClient MainLoop to continue.
     *
     * Returns true on an exit message, which will cause plClient to quit.
     */
    virtual bool ProcessEvents() = 0;

    /**
     * Does any necessary cleanup work to deinitialize the window.
     */
    virtual void DeInit() = 0;

    virtual hsWindowHndl GetWindowHandle() const = 0;
    virtual hsWindowHndl GetDisplayHandle() const = 0;
};


class plStubClientWindow : public plClientWindow
{
    typedef void (*plMessagePumpProc)();

private:
    hsWindowHndl fWindow;
    hsWindowHndl fDisplay;
    plMessagePumpProc fMessagePumpProc;

public:
    plStubClientWindow(hsWindowHndl w, hsWindowHndl d) : plClientWindow(), fWindow(w), fDisplay(d) { }

    void SetMessagePumpProc(plMessagePumpProc proc) { fMessagePumpProc = proc; }

    bool CreateClientWindow() override { return true; }
    void ShowClientWindow() override { }
    void ResizeClientWindow(uint16_t width, uint16_t height, bool windowed) override { }
    bool ProcessEvents() override { fMessagePumpProc(); return false; }
    void DeInit() override { }

    hsWindowHndl GetWindowHandle() const override {
        return fWindow;
    }
    hsWindowHndl GetDisplayHandle() const override {
        return fDisplay;
    }
};
