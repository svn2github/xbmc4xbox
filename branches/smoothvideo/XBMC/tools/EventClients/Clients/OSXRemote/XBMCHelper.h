//
//  XBMCHelper.h
//  xbmchelper
//
//  Created by Stephan Diederich on 11/12/08.
//  Copyright 2008 University Heidelberg. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "xbmcclientwrapper.h"

@class AppleRemote, MultiClickRemoteBehavior;

@interface XBMCHelper : NSObject {
  AppleRemote* mp_remote_control;
  XBMCClientWrapper* mp_wrapper;
  NSString* mp_app_path;
  NSString* mp_home_path;
  bool m_verbose;
}

- (void) enableVerboseMode:(bool) f_really;

- (void) setApplicationPath:(NSString*) fp_app_path;
- (void) setApplicationHome:(NSString*) fp_home_path;  

- (void) connectToServer:(NSString*) fp_server withMode:(eRemoteMode) f_mode;
- (void) setUniversalModeTimeout:(double) f_timeout;
- (void) disconnect;
@end
