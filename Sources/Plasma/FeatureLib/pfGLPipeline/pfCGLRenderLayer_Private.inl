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

// This is meant to be included in pfCGLRenderLayer.cpp

#define _GL_CLS(name) (Private::Class::s_k##name)
#define _GL_SEL(name) (Private::Selector::s_k##name)

#define _GL_DEF_CLS(symbol) struct objc_class* s_k##symbol = objc_lookUpClass(#symbol)
#define _GL_DEF_SEL(accessor, symbol) struct objc_selector* s_k##accessor = sel_registerName(symbol)

#define _GL_SUBCLASS_METHOD(name) \
    Method m_##name = class_getInstanceMethod(class_getSuperclass(getClass()), _GL_SEL(name)); \
    const char* me_##name = method_getTypeEncoding(m_##name); \
    class_addMethod(getClass(), _GL_SEL(name), (IMP)&pfCGLRenderLayer::name, me_##name);


namespace Private
{
    namespace Class
    {
        _GL_DEF_CLS(CAOpenGLLayer);
    }

    namespace Selector
    {
        _GL_DEF_SEL(layer,
            "layer");
        _GL_DEF_SEL(drawInCGLContext_pixelFormat_forLayerTime_displayTime_,
            "drawInCGLContext:pixelFormat:forLayerTime:displayTime:");
    }
}

template<typename _Ret, typename... _Args>
inline __attribute__((always_inline)) _Ret pfCGLRenderLayer::sendMsg(const void* pObj, struct objc_selector* selector, _Args... args)
{
    using SendMessageProc = _Ret (*)(const void*, struct objc_selector*, _Args...);
    const SendMessageProc proc = reinterpret_cast<SendMessageProc>(&objc_msgSend);

    return (*proc)(pObj, selector, args...);
}

template<typename _Ret, typename... _Args>
inline __attribute__((always_inline)) _Ret pfCGLRenderLayer::sendMsg(struct objc_selector* selector, _Args... args)
{
    using SendMessageProc = _Ret (*)(const void*, struct objc_selector*, _Args...);
    const SendMessageProc proc = reinterpret_cast<SendMessageProc>(&objc_msgSend);

    return (*proc)(this, selector, args...);
}

template<typename _Ret, typename... _Args>
inline __attribute__((always_inline)) _Ret pfCGLRenderLayer::sendSuper(struct objc_selector* selector, _Args... args)
{
    using SendMessageProc = _Ret (*)(const struct objc_super*, struct objc_selector*, _Args...);
    const SendMessageProc proc = reinterpret_cast<SendMessageProc>(&objc_msgSendSuper);

    struct objc_super super = {
        (objc_object*)this,
        class_getSuperclass(getClass())
    };

    return (*proc)(&super, selector, args...);
}
