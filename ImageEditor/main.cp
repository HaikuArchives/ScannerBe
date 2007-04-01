/* ScannerBe sample code. Copyright © Jim Moy, 1997, All rights reserved. */

#include "ScannerBe.h"
#include <Application.h>
#include <Bitmap.h>
#include <Window.h>
#include <stdio.h>

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

void ScanWindow::scan()
{
	status_t status = B_OK;
	scan_id id;
	const int32 kBufSize = 16 * 1024L;
	char *scanBuf = new char[ kBufSize ];
	
	scan_version version;
	status = scan_open( NULL, &id, &version );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't open the current scanner\n" );
		goto errXit;
	}

	// Kick off the scan.
	status = scan_start( id );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't start the scan (0x%X)\n", status );
		goto errXit;
	}
	
	// If you were scanning multiple images, as in the case of an automatic
	// document feeder, or some other mechanism by which the ScannerBe add-on
	// could provide multiple images in one session, each acquired image
	// would be wrapped by the scan_open_image()/scan_close_image() pair.
	status = scan_open_image( id );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't open the image (0x%X)\n", status );
		goto errXit;
	}
	
	// When scan_start() returns, the user has done their preview fiddled with
	// the settings, and then clicked the scan button. The scanner may go ahead
	// and do the scan, or it may wait until the first time we ask for the image
	// in scan_data() below. Always check for what's coming back here.
	scan_settings settings;
	status = scan_get_settings( id, &settings, NULL, NULL );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't get scanner settings (0x%X)\n", status );
		goto errXit;
	}
	
	// This demo app only handles gray & color images, one byte per sample,
	// but this is where you'd handle the threshold case and possibly handle
	// deeper than 24-bit color, etc.
	if( ( settings.image_type != SCAN_TYPE_RGB ) &&
			( settings.image_type != SCAN_TYPE_GRAY ) ) {
		fprintf( stderr, "Sorry, this app only handles gray or color data.\n" );
		goto errXit;
	}
	
	// Make the bitmap that we're going to draw.
	BRect bitmapRect(  0, 0, settings.pixel_width - 1, settings.pixel_height - 1 );
	color_space space = B_RGB_32_BIT;
	BBitmap *bitmap = new BBitmap( bitmapRect, space ) ;
	int32 bitmapSize = bitmap->BitsLength();
	
	// Do the scan loop, filling in the bitmap as we go.
	int32 offset = 0, count;
	for( bool notDone = true; notDone; ) {
		count = kBufSize;
		
		// scan data into scanBuf, get the number of bytes scanned back in count
		status = scan_data( id, scanBuf, &count );
		
		if( status != B_OK ) {
			// if we've reached the end of the scan, we get back B_SCANNER_DATA_END
			if( status == SCAN_DATA_END )
				status = B_OK;
			else				
				scan_close_image( id );			// some other scanner error
			notDone = false;
		}
		
		if( status == B_OK && count > 0 ) {
			if( settings.image_type == SCAN_TYPE_RGB ) {
				bitmap->SetBits( scanBuf, count, offset, space );
				offset += count * 4 / 3;
			} else {
				// scanner only does gray, but BBitmaps only do RGB
				char *ptr = (char *) scanBuf;
				char *dest = (char *) bitmap->Bits();
				dest += offset;
				for( int32 i = 0; i < count; i++, ptr++ ) {
					// (do your processing faster than this...)
					*dest++ = *ptr;		// B
					*dest++ = *ptr;		// G
					*dest++ = *ptr;		// R
					*dest++ = 0;		// A
				}
				offset += count * 4;
			}
		}
		
		// Do other things in the scan loop, like check for user
		// cancellation of the scan, etc.
	}
	
	status = scan_close_image( id );
	if( status != B_OK )
		fprintf( stderr, "Did not close the image (0x%X)\n", status );

errXit:
	delete[] scanBuf;
	status = scan_close( id );
	// At this point we're done using id, it doesn't mean anything any more.
	if( status != B_OK )
		fprintf( stderr, "Did not complete the scan (0x%X)\n", status );

	if( status == B_OK ) {
		_view->_bitmap = bitmap;
		_view->Invalidate();
	}
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
	((ScanWindow *) Window() )->scan();
	Window()->Activate();
}

