//
//  PLSView.m
//  plGLClient
//
//  Created by Colin Cornaby on 12/27/20.
//

#import "PLSView.h"
#include "plMessage/plInputEventMsg.h"

@interface PLSView ()

@property NSTrackingArea *mouseTrackingArea;

@end

@implementation PLSView

-(id)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)wantsLayer
{
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
}

-(void)mouseDown:(NSEvent *)event
{
    [self handleMouseButtonEvent:event];
}

-(void)mouseUp:(NSEvent *)event
{
    [self handleMouseButtonEvent:event];
}

-(void)mouseDragged:(NSEvent *)event
{
    [self updateClientMouseLocation:event];
}

-(void)rightMouseDown:(NSEvent *)event
{
    [self handleMouseButtonEvent:event];
}

-(void)rightMouseUp:(NSEvent *)event
{
    [self handleMouseButtonEvent:event];
}

-(void)rightMouseDragged:(NSEvent *)event
{
    [self updateClientMouseLocation:event];
}

-(void)mouseMoved:(NSEvent *)event {
    [self updateClientMouseLocation:event];
}

-(void)mouseEntered:(NSEvent *)event
{
    [NSCursor hide];
}

-(void)mouseExited:(NSEvent *)event
{
    //[super mouseExited:event];
    [NSCursor unhide];
}

-(void)updateTrackingAreas
{
    if(self.mouseTrackingArea) {
        [self removeTrackingArea:self.mouseTrackingArea];
    }
    self.mouseTrackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:NSTrackingMouseEnteredAndExited | NSTrackingActiveWhenFirstResponder owner:self userInfo:nil];
    [self addTrackingArea:self.mouseTrackingArea];
}

-(void)handleMouseButtonEvent:(NSEvent *)event {
    [self updateClientMouseLocation:event];
    
    CGPoint windowLocation = [event locationInWindow];
    CGPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
    
    plIMouseBEventMsg* pBMsg = new plIMouseBEventMsg;
    
    if(event.type == NSEventTypeLeftMouseUp) {
        pBMsg->fButton |= kLeftButtonUp;
    }
    else if(event.type == NSEventTypeRightMouseUp) {
        pBMsg->fButton |= kRightButtonUp;
    }
    else if(event.type == NSEventTypeLeftMouseDown) {
        pBMsg->fButton |= kLeftButtonDown;
    }
    else if(event.type == NSEventTypeRightMouseDown) {
        pBMsg->fButton |= kRightButtonDown;
    }
    
    self.inputManager->MsgReceive(pBMsg);
    
    delete(pBMsg);
}

-(void)updateClientMouseLocation:(NSEvent *)event {
    CGPoint windowLocation = [event locationInWindow];
    CGPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
    
    NSRect windowViewBounds = self.bounds;
    CGFloat deltaX = (windowLocation.x) / windowViewBounds.size.width;
    CGFloat deltaY = (windowViewBounds.size.height - windowLocation.y) / windowViewBounds.size.height;
    
    plIMouseXEventMsg* pXMsg = new plIMouseXEventMsg;
    plIMouseYEventMsg* pYMsg = new plIMouseYEventMsg;
    
    pXMsg->fWx = viewLocation.x;
    pXMsg->fX = deltaX;

    pYMsg->fWy = (windowViewBounds.size.height - windowLocation.y);
    pYMsg->fY = deltaY;
    
    self.inputManager->MsgReceive(pXMsg);
    self.inputManager->MsgReceive(pYMsg);
    
    delete(pXMsg);
    delete(pYMsg);
}

@end
