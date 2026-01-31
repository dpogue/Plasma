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

#ifndef _hsDarwin_inc_
#define _hsDarwin_inc_

#include "HeadSpin.h"

#ifdef HS_BUILD_FOR_APPLE
#include <string_theory/string>
#include <string_theory/format>
#include <CoreFoundation/CoreFoundation.h>
#include <objc/message.h>

#if !__has_feature(nullability)
#   ifndef _Nullable
#       define _Nullable
#   endif
#   ifndef _Nonnull
#       define _Nonnull
#   endif
#   ifndef _Null_unspecified
#       define _Null_unspecified
#   endif
#endif

template<typename T, typename U>
inline T bridge_cast(U* _Nullable obj)
{
#if defined(__OBJC__) && __has_feature(objc_arc)
    return (__bridge T)(obj);
#else
    return reinterpret_cast<T>(obj);
#endif
}

[[nodiscard]]
#if __has_feature(attribute_cf_returns_retained)
__attribute__((cf_returns_retained))
#endif
inline CFStringRef CFStringCreateWithSTString(const ST::string& str)
{
    return CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)str.data(), str.size(), kCFStringEncodingUTF8, false);
}

inline ST::string STStringFromCFString(CFStringRef str, ST::utf_validation_t validation = ST_DEFAULT_VALIDATION)
{
    CFRange range = CFRangeMake(0, CFStringGetLength(str));
    CFIndex strBufSz = 0;
    CFStringGetBytes(str, range, kCFStringEncodingUTF8, 0, false, nullptr, 0, &strBufSz);
    ST::char_buffer buffer;
    buffer.allocate(strBufSz);
    CFStringGetBytes(str, range, kCFStringEncodingUTF8, 0, false, (UInt8*)buffer.data(), strBufSz, nullptr);

    return ST::string(buffer, validation);
}

inline void format_type(const ST::format_spec &format, ST::format_writer &output, CFStringRef str)
{
    ST::char_buffer utf8 = STStringFromCFString(str).to_utf8();
    ST::format_string(format, output, utf8.data(), utf8.size());
}


#ifdef __OBJC__
#import <Foundation/Foundation.h>

#if !__has_feature(objc_instancetype)
#   undef instancetype
#   define instancetype id
#endif

@interface NSString (StringTheory)
+ (instancetype _Nullable)stringWithSTString:(const ST::string&)string;
- (instancetype _Nullable)initWithSTString:(const ST::string&)string;
- (const ST::string)STString;
@end


inline void format_type(const ST::format_spec &format, ST::format_writer &output, NSString* _Nonnull str)
{
    ST::char_buffer utf8 = STStringFromCFString(bridge_cast<CFStringRef>(str)).to_utf8();
    ST::format_string(format, output, utf8.data(), utf8.size());
}


#if __has_feature(objc_arc) || (MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 && defined(__clang__))
    extern "C" void* _Nonnull objc_autoreleasePoolPush(void);
    extern "C" void  objc_autoreleasePoolPop(void* _Nonnull pool);
#endif

class hsAutoreleasePool
{
private:
    void* _Nonnull const fPool;

public:
    hsAutoreleasePool()
#if __has_feature(objc_arc) || (MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 && defined(__clang__))
        : fPool(objc_autoreleasePoolPush())
#else
        : fPool([NSAutoreleasePool new])
#endif
    {}

    ~hsAutoreleasePool()
    {
#if __has_feature(objc_arc) || (MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 && defined(__clang__))
        objc_autoreleasePoolPop(fPool);
#else
        [bridge_cast<NSAutoreleasePool*>(fPool) drain];
#endif
    }
};

#define hsAutoreleasingScope hsAutoreleasePool hsUniqueIdentifier(_AutoreleasePool_);

#endif // __OBJC__

#endif // HS_BUILD_FOR_APPLE

#endif // _hsDarwin_inc_
