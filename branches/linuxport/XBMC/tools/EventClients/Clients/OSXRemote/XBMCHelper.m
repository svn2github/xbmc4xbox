//
//  XBMCHelper.m
//  xbmchelper
//
//  Created by Stephan Diederich on 11/12/08.
//  Copyright 2008 University Heidelberg. All rights reserved.
//

#import "XBMCHelper.h"
#import "remotecontrolwrapper/AppleRemote.h"
#import "XBMCDebugHelpers.h"
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
@implementation XBMCHelper
- (id) init{
  PRINT_SIGNATURE();
  if( ![super init] ){
    return nil;
  }
  mp_wrapper = nil;
  
  mp_remote_control = [[[AppleRemote alloc] initWithDelegate: self] retain];
  [mp_remote_control setProcessesBacklog:true];
  if( ! mp_remote_control ){
    NSException* myException = [NSException
                                exceptionWithName:@"AppleRemoteInitExecption"
                                reason:@"AppleRemote could not be initialized"
                                userInfo:nil];
    @throw myException;
  }
  [mp_remote_control startListening: self];
  return self;
}

- (void) dealloc{
  PRINT_SIGNATURE();
  [mp_remote_control release];
  [mp_wrapper release];
  [super dealloc];
}

//----------------------------------------------------------------------------
- (void) sendRemoteButtonEvent: (RemoteControlEventIdentifier) event pressedDown: (BOOL) pressedDown remoteControl: (RemoteControl*) remoteControl;
{
  if(m_verbose) {
    //do some logging here
    //[self logButton: event press;
  }
 
  switch(event){
    case kRemoteButtonPlay:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_PLAY];
      break;
    case kRemoteButtonPlay_Hold:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_PLAY_H];
      break;
    case kRemoteButtonRight:
      if(pressedDown) 
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_RIGHT_RELEASE];
      break;
    case kRemoteButtonRight_Hold:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_RIGHT_H];
      break;
    case kRemoteButtonLeft:
      if(pressedDown) 
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_LEFT_RELEASE];
      break;
    case kRemoteButtonLeft_Hold:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_LEFT_H];
      break;
    case kRemoteButtonPlus:
      if(pressedDown) 
        [mp_wrapper handleEvent:ATV_BUTTON_UP];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_UP_RELEASE];
      break;
    case kRemoteButtonMinus:
      if(pressedDown) 
        [mp_wrapper handleEvent:ATV_BUTTON_DOWN];
      else
        [mp_wrapper handleEvent:ATV_BUTTON_DOWN_RELEASE];
      break;      
    case kRemoteButtonMenu:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_MENU];
      break;
    case kRemoteButtonMenu_Hold:
      if(pressedDown) [mp_wrapper handleEvent:ATV_BUTTON_MENU_H];
      break;    
    case kRemoteControl_Switched:
      if(pressedDown) [mp_wrapper switchRemote: [mp_remote_control deviceID]];
        break;        
    default:
      NSLog(@"Oha, remote button not recognized %i pressed/released %i", event, pressedDown);
  }
}

//----------------------------------------------------------------------------
- (void) connectToServer:(NSString*) fp_server withMode:(eRemoteMode) f_mode{
 if(mp_wrapper)
   [self disconnect];
  mp_wrapper = [[XBMCClientWrapper alloc] initWithMode:f_mode serverAddress:fp_server];
  [mp_wrapper enableVerboseMode:m_verbose];
}

//----------------------------------------------------------------------------
- (void) setUniversalModeTimeout:(double) f_timeout{
  [mp_wrapper setUniversalModeTimeout:f_timeout];
}

//----------------------------------------------------------------------------
- (void) disconnect{
  [mp_wrapper release];
  mp_wrapper = nil;
}

//----------------------------------------------------------------------------
- (void) enableVerboseMode:(bool) f_really{
  m_verbose = f_really;
  [mp_wrapper enableVerboseMode:f_really];
}

 //   NSString* pressed;
//    NSString* buttonName;
//    if (pressedDown) pressed = @"(pressed)"; else pressed = @"(released)";
//    
//    switch(event) {
//      case kRemoteButtonPlus:
//        buttonName = @"Volume up";			
//        break;
//      case kRemoteButtonMinus:
//        buttonName = @"Volume down";
//        break;			
//      case kRemoteButtonMenu:
//        buttonName = @"Menu";
//        break;			
//      case kRemoteButtonPlay:
//        buttonName = @"Play";
//        break;			
//      case kRemoteButtonRight:	
//        buttonName = @"Right";
//        break;			
//      case kRemoteButtonLeft:
//        buttonName = @"Left";
//        break;			
//      case kRemoteButtonRight_Hold:
//        buttonName = @"Right holding";	
//        break;	
//      case kRemoteButtonLeft_Hold:
//        buttonName = @"Left holding";		
//        break;			
//      case kRemoteButtonPlus_Hold:
//        buttonName = @"Volume up holding";	
//        break;				
//      case kRemoteButtonMinus_Hold:			
//        buttonName = @"Volume down holding";	
//        break;				
//      case kRemoteButtonPlay_Hold:
//        buttonName = @"Play (sleep mode)";
//        break;			
//      case kRemoteButtonMenu_Hold:
//        buttonName = @"Menu holding";
//        break;
//      case kRemoteControl_Switched:
//        buttonName = @"Remote Control Switched";
//        break;
//      default:
//        break;
//    }
//    NSLog(@"%@ %@", pressed, buttonName);
//    }


@end
