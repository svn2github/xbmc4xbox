//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

void* InitializeAutoReleasePool()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	return pool;
}

void DestroyAutoReleasePool(void* aPool)
{
	NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
	[pool release];
}

void Cocoa_GL_MakeCurrentContext(void* theContext)
{
	NSOpenGLContext* context = (NSOpenGLContext* )theContext;
	[ context makeCurrentContext ];
}

void Cocoa_GL_ReleaseContext(void* theContext)
{
	NSOpenGLContext* context = (NSOpenGLContext* )theContext;
  [ NSOpenGLContext clearCurrentContext ];
  [ context clearDrawable ];
  [ context release ];
}

void Cocoa_GL_SwapBuffers(void* theContext)
{
	NSOpenGLContext* context = (NSOpenGLContext* )theContext;
	[ context flushBuffer ];
}