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

#import "PLSKeyboardEventMonitor.h"
#include "plMessage/plInputEventMsg.h"

@interface PLSKeyboardEventMonitor ()

@property (weak) NSView *view;
@property plInputManager *inputManager;
@property (retain) id localMonitor;

@end

@implementation PLSKeyboardEventMonitor

NSEventMask eventMasks =
NSEventMaskKeyDown |
NSEventMaskKeyUp |
NSEventMaskFlagsChanged;

-(id)initWithView:(NSView *)view inputManager:(plInputManager*)inputManager
{
    self = [super init];
    self.view = view;
    self.inputManager = inputManager;
    
    self.localMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:eventMasks handler:^NSEvent * _Nullable(NSEvent * _Nonnull event) {
        if([self processEvent:event]) {
            return nil;
        }
        return event;
    }];
    
    return self;
}

-(BOOL)processEvent:(NSEvent *)event {
    //is this even an event for our window
    if([event window] == [self.view window]) {
        switch(event.type) {
            case NSEventTypeKeyDown:
            case NSEventTypeKeyUp:
            case NSEventTypeFlagsChanged: {
                [self processKeyEvent:event];
                break;
            }
            default:
                NSLog(@"Unexpected unhandled event type %@", event);
                return NO;
        }
        if(event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp) {
            return YES;
        }
    }
    return NO;
}

-(void)processKeyEvent:(NSEvent *)event {
    if(event.type == NSEventTypeKeyDown && event.ARepeat) {
        return;
    }
    
    const unsigned short KEYCODE_LINUX_TO_HID[0x7F] = {
         4, 22,  7,  9, 11, 10, 29, 27, 06, 25, 00, 05, 20, 26,  8, 21,
        //0x10
        28, 23, 30, 31, 32, 33, 35, 34, 46, 38, 36, 45, 37, 39, 48, 18,
        //0x20
        24, 47, 12, 19,158, 15, 13, 52, 14, 51, 49, 54, 56, 17, 16, 55,
        //0x30
        43, 44, 53, 42, 41, 0, 0, 0, 225, 57, 226,
        //0x3B
        224, 0, 0, 0, 0,
        //0x40
        00, 99, 00, 85, 00, 87, 00, 00, 00, 00, 00, 84, 158, 0, 86, 00,
        //0x50
        00, 00, 98, 89, 90, 91, 92, 93, 94, 95, 00, 96, 97,
        //0x5D
        0, 0, 0,
        //0x60
        62, 63, 64, 60, 65, 66, 00, 68, 00, 00, 00, 00, 00, 67, 00, 69,
        //0x70
        00, 00, 00, 74, 75, 76, 61, 77, 59, 78, 58, 80, 79, 81, 82
    };
    
    BOOL down = event.type == NSEventTypeKeyDown;
    
    unsigned short keycode = [event keyCode];
    plKeyDef convertedKey = (plKeyDef)KEYCODE_LINUX_TO_HID[keycode];
    
    NSEventModifierFlags modifierFlags = [event modifierFlags];
    //if it's a shift key event only way to derive up or down is through presence in the modifier flag
    if(convertedKey == KEY_SHIFT) {
        down = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    }
    if(convertedKey == KEY_ALT) {
        down = (event.modifierFlags & NSEventModifierFlagOption) != 0;
    }
    if(convertedKey == KEY_CTRL) {
        down = (event.modifierFlags & NSEventModifierFlagControl) != 0;
    }
    //NSLog(@"Key event %@, Plasma key is %i, is down %i", event, convertedKey, down);
    self.inputManager->HandleKeyEvent(convertedKey, down, false);
}

@end
