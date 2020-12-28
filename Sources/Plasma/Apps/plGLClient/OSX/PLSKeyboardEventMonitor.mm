//
//  PLSEventMonitor.m
//  plGLClient
//
//  Created by Colin Cornaby on 12/24/20.
//

#import "PLSKeyboardEventMonitor.h"
#include "plMessage/plInputEventMsg.h"
#import <Carbon/Carbon.h>

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
