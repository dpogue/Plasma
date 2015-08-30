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
#include "hsWindows.h"

#include "plGLClient/plGLClient.h"

#include "plResMgr/plResManager.h"

#include "plProduct.h"

#define CLASSNAME "Plasma"

int gWinBorderDX    = GetSystemMetrics( SM_CXSIZEFRAME );
int gWinBorderDY    = GetSystemMetrics( SM_CYSIZEFRAME );
int gWinMenuDY      = GetSystemMetrics( SM_CYCAPTION );

plClient* gClient = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Handle messages
    switch (message) {
        case WM_CLOSE:
            gClient->SetDone(TRUE);
            DestroyWindow(gClient->GetWindowHandle());
            break;
        case WM_DESTROY:
            gClient->SetDone(TRUE);
            PostQuitMessage(0);
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    // Fill out WNDCLASS info
    WNDCLASS wndClass;
    wndClass.style              = CS_DBLCLKS;   // CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc        = WndProc;
    wndClass.cbClsExtra         = 0;
    wndClass.cbWndExtra         = 0;
    wndClass.hInstance          = hInst;
    wndClass.hIcon              = LoadIcon(NULL, IDI_APPLICATION);

    wndClass.hCursor            = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground      = (struct HBRUSH__*) (GetStockObject(BLACK_BRUSH));
    wndClass.lpszMenuName       = CLASSNAME;
    wndClass.lpszClassName      = CLASSNAME;

    if (!RegisterClass(&wndClass)) {
        return 1;
    }

    // Create a window
    HWND hWnd = CreateWindow(
        CLASSNAME, plProduct::LongName().c_str(),
        WS_OVERLAPPEDWINDOW,
        0, 0,
        800 + gWinBorderDX * 2,
        600 + gWinBorderDY * 2 + gWinMenuDY,
         NULL, NULL, hInst, NULL
    );


    plResManager *resMgr = new plResManager();
    resMgr->SetDataPath("dat");
    hsgResMgr::Init(resMgr);

    gClient = new plClient();
    if (gClient == nullptr)
    {
        return 1;
    }

    gClient->SetWindowHandle((hsWindowHndl)hWnd);

    HDC hDC = GetDC(hWnd);

    if (gClient->InitPipeline((hsWindowHndl)hDC))
    {
        return 1;
    }

    if (!gClient->StartInit())
    {
        return 1;
    }

    ShowWindow(gClient->GetWindowHandle(), SW_SHOW);
    BringWindowToTop(gClient->GetWindowHandle());
    UpdateWindow(gClient->GetWindowHandle());

    MSG msg;
    do
    {
        gClient->MainLoop();

        if (gClient->GetDone()) {
            break;
        }

        // Look for a message
        while (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            // Handle the message
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    } while (WM_QUIT != msg.message);


    if (gClient)
    {
        gClient->Shutdown();
        gClient = nullptr;
    }

    hsAssert(hsgResMgr::ResMgr()->RefCnt()==1, "resMgr has too many refs, expect mem leaks");
    hsgResMgr::Shutdown(); // deletes fResMgr

    return 0;
}
