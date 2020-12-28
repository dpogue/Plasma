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
#include "plInputCore/plInputManager.h"
#include "plMessage/plInputEventMsg.h"
#include "plPipeline/GL/plGLPipeline.h"

#include "pfConsoleCore/pfConsoleEngine.h"

#import "Cocoa/Cocoa.h"
#import <OpenGL/gl.h>
#import <QuartzCore/QuartzCore.h>
#import "PLSKeyboardEventMonitor.h"


void PumpMessageQueueProc();

@interface AppDelegate: NSWindowController <NSApplicationDelegate, NSWindowDelegate> {
    @public plClientLoader gClient;
    
}

@property (retain) NSTimer *drawTimer;
@property (retain) PLSKeyboardEventMonitor *eventMonitor;
@property const char **argv;
@property int argc;

@end

@implementation AppDelegate
PF_CONSOLE_LINK_ALL()

-(id)init {
    
    // Style flags
    NSUInteger windowStyle =
    (NSWindowStyleMaskTitled  |
         NSWindowStyleMaskClosable |
         NSWindowStyleMaskResizable);
    
    // Window bounds (x, y, width, height)
    NSRect windowRect = NSMakeRect(100, 100, 800, 600);
    
    NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect
                        styleMask:windowStyle
                        backing:NSBackingStoreBuffered
                        defer:NO];
    
    self = [super initWithWindow:window];
    self.window.acceptsMouseMovedEvents = YES;
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    PF_CONSOLE_INITIALIZE(Audio)
    // Create a window:

    // Window controller
    [self.window setContentSize:NSMakeSize(800, 600)];
    [self.window orderFrontRegardless];
    
    gClient.SetClientWindow((hsWindowHndl)(__bridge void *)self.window);
    gClient.SetClientDisplay((hsWindowHndl)NULL);
    gClient.Init(_argc, _argv);
    
    // We should quite frankly be done initing the client by now. But, if not, spawn the good old
    // "Starting URU, please wait..." dialog (not so yay)
    while (!gClient.IsInited())
    {
        [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate now]];
    }
    
    [self.window setTitle:[NSString stringWithCString:plProduct::LongName().c_str() encoding:NSUTF8StringEncoding]];
    
    if(!gClient) {
        exit(0);
    }
    
    self.eventMonitor = [[PLSKeyboardEventMonitor alloc] initWithView:self.window.contentView inputManager:gClient->GetInputManager()];

    // Main loop
    if (gClient && !gClient->GetDone())
    {
        gClient->SetMessagePumpProc(PumpMessageQueueProc);
        gClient.Start();
        CVDisplayLinkRef displayLink;
        CVDisplayLinkCreateWithCGDisplay([self.window.screen.deviceDescription[@"NSScreenNumber"] intValue], &displayLink);
        CVDisplayLinkSetOutputHandler(displayLink, ^CVReturn(CVDisplayLinkRef  _Nonnull displayLink, const CVTimeStamp * _Nonnull inNow, const CVTimeStamp * _Nonnull inOutputTime, CVOptionFlags flagsIn, CVOptionFlags * _Nonnull flagsOut) {
            dispatch_sync(dispatch_get_main_queue(), ^{
                gClient->MainLoop();
                PumpMessageQueueProc();

                if (gClient->GetDone()) {
                    gClient.ShutdownEnd();
                    [NSApp terminate:self];
                }
            });
            return kCVReturnSuccess;
        });
        CVDisplayLinkStart(displayLink);
    }
}

@end

void PumpMessageQueueProc()
{
    /*
     Normally we want to receive events from the normal app event loop,
     but the intro movie blocks the normal event loop. This lets us
     manually process key down during the intro.
     */
    plClientLoader &gClient = ((AppDelegate *)[NSApp delegate])->gClient;
    if(gClient->GetQuitIntro() == false) {
        NSEvent *event;
        [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate now]];
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskKeyDown
                                    untilDate:[NSDate distantPast]
                                    inMode:NSDefaultRunLoopMode
                                    dequeue:YES]) != nil)
        {
            switch ([event type])
            {
            case NSEventTypeKeyDown:
                {
                    gClient->SetQuitIntro(true);
                }
                break;
            default:
                break;
            }
        }
    }
}

int main(int argc, const char** argv)
{
    // Create a shared app instance.
    // This will initialize the global variable
    // 'NSApp' with the application instance.
        //[application setDelegate:delegate];
    @autoreleasepool {
        AppDelegate *delegate = [AppDelegate new];
        delegate.argv = argv;
        delegate.argc = argc;
        
        NSMenu *mainMenu = [[NSMenu alloc] init];
        NSApplication * application = [NSApplication sharedApplication];
        
        
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        application.mainMenu = mainMenu;
        application.delegate = delegate;
        [application run];
    }
    return 0;
}
