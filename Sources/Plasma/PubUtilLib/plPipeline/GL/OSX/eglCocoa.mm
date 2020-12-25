/* Copyright (C) 2011  Nokia Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#import <AppKit/NSOpenGL.h>
#import <AppKit/NSView.h>
#import <AppKit/NSWindow.h>
#import <Foundation/NSGeometry.h>
#import <Foundation/NSNotification.h>
#import "eglCocoa.h"

@interface CocoaView : NSObject
{
  @private
    NSView* mView;
    NSOpenGLContext* mContext;
}

- (id)initWithView:(NSView*)view;
- (void)activateInContext:(NSOpenGLContext*)context;
- (void)frameChanged:(NSNotification*)notification;
@end

@implementation CocoaView
- (id)initWithView:(NSView*)view
{
    if ((self = [super init]) != nil) {
        mView = view ;
        mContext = nil;
        [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(frameChanged:)
            name:NSViewGlobalFrameDidChangeNotification
            object:mView];
        //[mView setPostsFrameChangedNotifications:YES];
        return self;
    }
    return nil;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (mContext != nil) {
        mContext = nil;
    }
    if (mView != nil) {
        mView = nil;
    }
}

- (void)finalize
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (mContext != nil) {
        mContext = nil;
    }
    if (mView != nil) {
        mView = nil;
    }
    [super finalize];
}

- (void)activateInContext:(NSOpenGLContext*)context
{
    mContext = context;
    if([context view] != mView) {
        dispatch_sync(dispatch_get_main_queue(), ^{
            [mContext setView:mView];
        });
    }
}

- (void)frameChanged:(NSNotification*)notification
{
    if (mContext != nil && [mContext view] == mView) {
        [mContext update];
    }
}
@end



void* CreateContext(CGLContextObj ctx)
{
    return (void *)CFBridgingRetain([[NSOpenGLContext alloc] initWithCGLContextObj:ctx]);
}

void DestroyContext(void* nsctx)
{
    //[(NSOpenGLContext *)nsctx release];
}

void* CreateView(EGLSurface s, void* nsview, unsigned* width, unsigned* height)
{
    __block CocoaView *cocoaView;
    //Dispatch sync onto the main queue
    //IF THIS IS CALLED FROM THE MAIN QUEUE COULD CAUSE A DEADLOCK
    //Hack hack hack until we can get this code sorted out better
    //So far in use this function has not been called from the main thread
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSWindow* w = (__bridge NSWindow*)nsview;
        NSView* v = [w contentView];
        NSRect bounds = [v bounds];

        if (width) {
            *width  = bounds.size.width;
        }
        if (height) {
            *height = bounds.size.height;
        }
        cocoaView = [[CocoaView alloc] initWithView:v];
    });

    return (void *)CFBridgingRetain(cocoaView);
}

void DestroyView(void* cview)
{
    [[NSNotificationCenter defaultCenter] removeObserver:(__bridge id)cview];
}

void SetView(void* nsctx, void* cview)
{
    [(__bridge CocoaView*)cview activateInContext:(__bridge NSOpenGLContext*)nsctx];
}
