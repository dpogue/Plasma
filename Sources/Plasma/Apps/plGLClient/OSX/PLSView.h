//
//  PLSView.h
//  plGLClient
//
//  Created by Colin Cornaby on 12/27/20.
//

#import <Cocoa/Cocoa.h>
#include "plInputCore/plInputManager.h"

NS_ASSUME_NONNULL_BEGIN

@interface PLSView : NSView

@property plInputManager * inputManager;

@end

NS_ASSUME_NONNULL_END
