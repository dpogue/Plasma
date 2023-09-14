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

#include "pfCGLRenderLayer.h"

#include <functional>

#include "plCGLDevice.h"
#include "pfCGLRenderLayer_Private.inl"

struct objc_class* pfCGLRenderLayer::sRenderLayerClass = nullptr;

void pfCGLRenderLayer::IInitClass()
{
    sRenderLayerClass = objc_allocateClassPair(_GL_CLS(CAOpenGLLayer), "pfCGLRenderLayer", 0);

    class_addIvar(sRenderLayerClass, "_data", sizeof(RenderLayerData), log2(sizeof(RenderLayerData)), "{?=^v}");

    _GL_SUBCLASS_METHOD(drawInCGLContext_pixelFormat_forLayerTime_displayTime_);

    objc_registerClassPair(sRenderLayerClass);
}

pfCGLRenderLayer* pfCGLRenderLayer::getLayer()
{
    if (!getClass()) {
        IInitClass();
    }

    pfCGLRenderLayer* layer = sendMsg<pfCGLRenderLayer*>(getClass(), _GL_SEL(layer));

    RenderLayerData& data = layer->getRenderLayerData();
    data.device = nullptr;

    return layer;
}

void pfCGLRenderLayer::SetDevice(plGLDevice& dev)
{
    RenderLayerData& data = getRenderLayerData();
    data.device = reinterpret_cast<plCGLDevice*>(dev.fImpl);
}

void pfCGLRenderLayer::drawInCGLContext_pixelFormat_forLayerTime_displayTime_(pfCGLRenderLayer* self, struct obj_selector* cmd, CGLContextObj ctx, CGLPixelFormatObj pf, CFTimeInterval t, const CVTimeStamp* ts)
{
    CGLSetCurrentContext(ctx);

    glClearColor(1.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    self->sendSuper<void>(_GL_SEL(drawInCGLContext_pixelFormat_forLayerTime_displayTime_), ctx, pf, t, ts);
}
