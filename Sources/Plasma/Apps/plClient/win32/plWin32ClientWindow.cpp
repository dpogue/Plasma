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

#include "plWin32ClientWindow.h"
#include "res/resource.h"
#include "plProduct.h"
#include "plWinDpi/plWinDpi.h"

#define CLASSNAME "Plasma"

LRESULT CALLBACK ClientWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    plWin32ClientWindow* cw = reinterpret_cast<plWin32ClientWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (cw) {
        std::optional<LRESULT> result = cw->WndProc(message, wParam, lParam);
        if (result.has_value())
            return result.value();
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool plWin32ClientWindow::PreInit()
{
    // Initialize the DPI helpers
    plWinDpi::Instance();

    return true;
}

bool plWin32ClientWindow::CreateClientWindow()
{
    // Fill out WNDCLASS info
    WNDCLASS wndClass;
    wndClass.style = CS_DBLCLKS;   // CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = ClientWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = fInst;
    wndClass.hIcon = LoadIcon(fInst, MAKEINTRESOURCE(IDI_ICON_DIRT));
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (struct HBRUSH__*) (GetStockObject(BLACK_BRUSH));
    wndClass.lpszMenuName = CLASSNAME;
    wndClass.lpszClassName = CLASSNAME;

    // can only run one at a time anyway, so just quit if another is running
    if (!RegisterClass(&wndClass))
        return false;

    int winBorderDX = plWinDpi::Instance().GetSystemMetrics(SM_CXSIZEFRAME);
    int winBorderDY = plWinDpi::Instance().GetSystemMetrics(SM_CYSIZEFRAME);
    int winMenuDY = plWinDpi::Instance().GetSystemMetrics(SM_CYCAPTION);

    // Create a window
    fWnd = CreateWindow(
        CLASSNAME, plProduct::LongName().c_str(),
        WS_OVERLAPPEDWINDOW,
        0, 0,
        800 + winBorderDX * 2,
        600 + winBorderDY * 2 + winMenuDY,
        nullptr, nullptr, fInst, nullptr
        );
    SetWindowLongPtr(fWnd, GWLP_USERDATA, (LONG_PTR)this);
    fDC = GetDC(fWnd);

    return true;
}

void plWin32ClientWindow::ShowClientWindow()
{
    ShowWindow(fWnd, SW_SHOW);
    BringWindowToTop(fWnd);
}

void plWin32ClientWindow::ResizeClientWindow(uint16_t width, uint16_t height, bool windowed)
{
    uint32_t winStyle, winExStyle;
    if (windowed) {
        // WS_VISIBLE appears necessary to avoid leaving behind framebuffer junk when going from windowed to a smaller window
        winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        winExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    } else {
        winStyle = WS_VISIBLE;
        winExStyle = WS_EX_APPWINDOW;
    }
    SetWindowLongPtr(fWnd, GWL_STYLE, winStyle);
    SetWindowLongPtr(fWnd, GWL_EXSTYLE, winExStyle);

    uint32_t flags = SWP_NOCOPYBITS | SWP_SHOWWINDOW | SWP_FRAMECHANGED;

    // The window rect will be (left, top, width, height)
    RECT winRect{ 0, 0, width, height };
    if (windowed) {
        if (GetClientRect(fWnd, &winRect) != FALSE) {
            MapWindowPoints(fWnd, nullptr, reinterpret_cast<LPPOINT>(&winRect), 2);
            winRect.right = winRect.left + width;
            winRect.bottom = winRect.top + height;
        }

        UINT dpi = plWinDpi::Instance().GetDpi(fWnd);
        plWinDpi::Instance().AdjustWindowRectEx(&winRect, winStyle, false, winExStyle, dpi);

        winRect.right = winRect.right - winRect.left;
        winRect.bottom = winRect.bottom - winRect.top;
    }
    SetWindowPos(fWnd, HWND_NOTOPMOST, winRect.left, winRect.top, winRect.right, winRect.bottom, flags);
}

bool plWin32ClientWindow::ProcessEvents()
{
    MSG msg;

    // Look for a message
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        // Handle the message
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.message == WM_QUIT;
}

std::optional<LRESULT> plWin32ClientWindow::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    return std::nullopt;
}
