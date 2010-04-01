/* Copyright (c) 2006-2010, Linden Research, Inc.
 * 
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include "ubrowser.h"

#ifdef LL_OSX
#include "GLUT/glut.h"
#include "glui.h"
#else
#include "GL/glut.h"
#include "GL/glui.h"
#endif

uBrowser* theApp;

////////////////////////////////////////////////////////////////////////////////
//
void glutReshape( int widthIn, int heightIn )
{
	if ( theApp )
		theApp->reshape( widthIn, heightIn );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutDisplay()
{
	if ( theApp )
		theApp->display();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutIdle()
{
	if ( theApp )
		theApp->idle();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboard( unsigned char keyIn, int xIn, int yIn )
{
	if ( theApp )
		theApp->keyboard( keyIn, xIn, yIn );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutSpecialKeyboard( int keyIn, int xIn, int yIn )
{
	if ( theApp )
		theApp->specialKeyboard( keyIn, xIn, yIn );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutPassiveMouse( int xIn, int yIn )
{
	if ( theApp )
		theApp->passiveMouse( xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseMove( int xIn , int yIn )
{
	if ( theApp )
		theApp->mouseMove( xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseButton( int buttonIn, int stateIn, int xIn, int yIn )
{
	if ( theApp )
		theApp->mouseButton( buttonIn, stateIn, xIn, yIn );
}

////////////////////////////////////////////////////////////////////////////////
//
int main( int argc, char* argv[] )
{
	theApp = new uBrowser;

	if ( theApp )
	{
		glutInit( &argc, argv );
		glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );

		glutInitWindowPosition( 80, 0 );
		glutInitWindowSize( 1024, 900 );

		int appWindow = glutCreateWindow( theApp->getName().c_str() );

		glutDisplayFunc( glutDisplay );

		GLUI_Master.set_glutReshapeFunc( glutReshape );
		GLUI_Master.set_glutKeyboardFunc( glutKeyboard );
		GLUI_Master.set_glutMouseFunc( glutMouseButton );
		GLUI_Master.set_glutSpecialFunc( glutSpecialKeyboard );

		glutPassiveMotionFunc( glutPassiveMouse );
		glutMotionFunc( glutMouseMove );

		glutSetWindow( appWindow );

		theApp->init( argv[ 0 ], appWindow );

		GLUI_Master.set_glutIdleFunc( glutIdle );

		glutMainLoop();

		delete theApp;
	};

	return 1;
}

