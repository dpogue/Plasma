//
//  PLSEventMonitor.h
//  plGLClient
//
//  Created by Colin Cornaby on 12/24/20.
//

#import <Cocoa/Cocoa.h>
#include "plProduct.h"
#include "plGLClient/plGLClient.h"
#include "plGLClient/plClientLoader.h"
#include "plInputCore/plInputManager.h"

NS_ASSUME_NONNULL_BEGIN

@interface PLSKeyboardEventMonitor : NSObject

-(id)initWithView:(NSView *)view inputManager:(plInputManager*)inputManager;

@end

NS_ASSUME_NONNULL_END
