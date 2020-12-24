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

#include "plGLClient/plGLClient.h"
#include "plGLClient/plClientLoader.h"

#import "Cocoa/Cocoa.h"
#import <OpenGL/gl.h>

plClientLoader gClient = plClientLoader();

void PumpMessageQueueProc();

@interface AppDelegate: NSObject <NSApplicationDelegate> {
    
}

@property (retain) NSTimer *drawTimer;
@property const char **argv;
@property int argc;

@end

@implementation AppDelegate
PF_CONSOLE_LINK_ALL()

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // Create a window:

    // Style flags
    NSUInteger windowStyle =
        (NSTitledWindowMask  |
        NSClosableWindowMask |
        NSResizableWindowMask);

    // Window bounds (x, y, width, height)
    NSRect windowRect = NSMakeRect(100, 100, 800, 600);

    NSWindow * window = [[[NSWindow alloc] initWithContentRect:windowRect
                        styleMask:windowStyle
                        backing:NSBackingStoreBuffered
                        defer:NO] autorelease];

    // Window controller
    NSWindowController * windowController = [[[NSWindowController alloc] initWithWindow:window] autorelease];

    [window setTitle:@"Uru"];
    [window setContentSize:NSMakeSize(800, 600)];
    [window orderFrontRegardless];
    
    gClient.SetClientWindow((hsWindowHndl)window);
    PF_CONSOLE_INITIALIZE(Audio);
    gClient.SetClientDisplay((hsWindowHndl)NULL);
    gClient.Init(_argc, _argv);
    //NSApp.mainWindow = window;
    //[NSApp run];
    
    // We should quite frankly be done initing the client by now. But, if not, spawn the good old
    // "Starting URU, please wait..." dialog (not so yay)
    while (!gClient.IsInited())
    {
        [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate now]];
    }
    
    if(!gClient) {
        exit(0);
    }

    // Can't do this threaded on mac
    if (gClient->InitPipeline(NULL) ||
        !gClient->StartInit()) {
        gClient->SetDone(true);
    }

    gClient->SetQuitIntro(true);


    // Main loop
    if (gClient && !gClient->GetDone())
    {
        gClient->SetMessagePumpProc(PumpMessageQueueProc);
        gClient.Start();
        self.drawTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0 repeats:true block:^(NSTimer * _Nonnull timer) {
            gClient->MainLoop();
            PumpMessageQueueProc();

            if (gClient->GetDone()) {
                gClient.ShutdownEnd();
                [NSApp terminate:self];
            }
        }];
        
    }



    //[pool drain];
}

@end

void PumpMessageQueueProc()
{
    NSEvent* event;

    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                            untilDate:[NSDate distantPast]
                            inMode:NSDefaultRunLoopMode
                            dequeue:YES]) != nil)
    {

        switch ([event type])
        {
        case NSEventTypeKeyDown:
            {
                gClient->SetDone(true);
            }
            break;
        default:
            break;
        }

        [NSApp sendEvent:event];
        [NSApp updateWindows];
    }
}


int main(int argc, const char** argv)
{
    // Autorelease Pool:
    // Objects declared in this scope will be automatically
    // released at the end of it, when the pool is "drained".
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    // Create a shared app instance.
    // This will initialize the global variable
    // 'NSApp' with the application instance.
        //[application setDelegate:delegate];
    
    AppDelegate *delegate = [AppDelegate new];
    delegate.argv = argv;
    delegate.argc = argc;
    
    NSMenu *mainMenu = [[NSMenu alloc] init];
    NSApplication * application = [NSApplication sharedApplication];
    
    
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    application.mainMenu = mainMenu;
    application.delegate = delegate;
    [application run];

    

    return 0;
}
