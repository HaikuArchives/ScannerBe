/* ScannerBe sample code. Copyright © Jim Moy, 1997, All rights reserved. */

#include "ScannerBe.h"
#include <Application.h>
#include <Bitmap.h>
#include <Window.h>
#include <stdio.h>
#include "ScanGlue.h"

const char *kSig = "application/x-vnd.jbm-guiscandemo";
const char *kSignature = "application/x-vnd.jbm-scan";

#pragma mark ---- Class Declarations ----

class ScanView : public BView {
public:
					ScanView( BRect rect );
virtual	void		Draw( BRect updateRect );
virtual void		MouseDown( BPoint where );

		BBitmap*	_bitmap;
};

class ScanWindow : public BWindow {
public:
					ScanWindow();
virtual	bool		QuitRequested();
		void		scan();
		
		ScanView*	_view;
};

class ScanApp : public BApplication {
public:
					ScanApp();

		ScanWindow*	_window;
};

void main() { ScanApp app; app.Run(); }

#pragma mark ---- Functions ----

ScanApp::ScanApp() : BApplication( kSig )
{
	_window = new ScanWindow;
}

ScanWindow::ScanWindow()
	: BWindow( BRect( 100, 100, 400, 500 ), "Scanned Image", B_TITLED_WINDOW, 0 )
{
	BRect viewRect( Bounds() );
	_view = new ScanView( viewRect );
	AddChild( _view );
	Show();
}

bool ScanWindow::QuitRequested()
{
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}

ScanView::ScanView( BRect frame ) : BView( frame, "", B_FOLLOW_ALL, B_WILL_DRAW )
{
	_bitmap = NULL;
}

void ScanView::Draw( BRect updateRect )
{
	if( _bitmap )
		DrawBitmap( _bitmap, BPoint( 0, 0 ) );
	else
		DrawString( "Click here to scan.", BPoint( 20, 40 ) );
}

void ScanView::MouseDown( BPoint where )
{
	// Every time someone clicks in the window, a new scan is invoked.
	delete _bitmap;
	_bitmap = NULL;
	status_t status;
	_bitmap = GetScannerImage( status );
	Invalidate();
	Window()->Activate();
}

