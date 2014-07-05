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

#include "plGLDevice.h"


plGLDevice::plGLDevice()
:   fErrorMsg(nullptr),
    fDisplay(EGL_NO_DISPLAY),
    fSurface(EGL_NO_SURFACE),
    fContext(EGL_NO_CONTEXT)
{
}


bool plGLDevice::InitDevice()
{
    if (!eglBindAPI(EGL_OPENGL_API))
    {
        fErrorMsg = "Could not bind to correct API";
        return false;
    }

    /* Set up the display */
    fDisplay = eglGetDisplay((EGLNativeDisplayType)fDevice);
    if (fDisplay == EGL_NO_DISPLAY)
    {
        fErrorMsg = "Could not get the display";
        return false;
    }

    if (!eglInitialize(fDisplay, nullptr, nullptr))
    {
        fErrorMsg = "Could not initialize the display";
        return false;
    }


    /* Set up the config attributes for EGL */
    EGLConfig  config;
    EGLint     config_count;
    EGLint config_attrs[] = {
        EGL_BUFFER_SIZE, 16,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_BIT,
        EGL_NONE
    };

    if (!eglChooseConfig(fDisplay, config_attrs, &config, 1, &config_count) || config_count != 1)
    {
        fErrorMsg = "Could not choose appropriate config";
        return false;
    }


    /* Set up the rendering surface */
    fSurface = eglCreateWindowSurface(fDisplay, config, (EGLNativeWindowType)fWindow, nullptr);
    if (fSurface == EGL_NO_SURFACE)
    {
        fErrorMsg = "Unable to create rendering surface";
        return false;
    }


    /* Set up the GL context */
    EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    fContext = eglCreateContext(fDisplay, config, EGL_NO_CONTEXT, ctx_attrs);
    if (fContext == EGL_NO_CONTEXT)
    {
        fErrorMsg = "Unable to create rendering context";
        return false;
    }


    /* Associate everything */
    eglMakeCurrent(fDisplay, fSurface, fSurface, fContext);

    return true;
}

void plGLDevice::SetRenderTarget(plRenderTarget* target)
{
}

void plGLDevice::SetViewport()
{
}


bool plGLDevice::EndRender()
{
    eglSwapBuffers(fDisplay, fSurface);
    return true;
}

void plGLDevice::SetProjectionMatrix(const hsMatrix44& src)
{
}

void plGLDevice::SetWorldToCameraMatrix(const hsMatrix44& src)
{
}

void plGLDevice::SetLocalToWorldMatrix(const hsMatrix44& src)
{
}
