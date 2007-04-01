// ScanGlue.cpp - Some wrapper routines around the ScannerBe interface.
//
// Copyright © 1997, Jim Moy, Please munge this code to work for you!

/*
	GetScannerImage()

		This is the basic, "I want to include scanning support,
		but I don't want to do any work" method. Just call it,
		and you'll have an image when the scanning is done. You
		may want to put this in a separate thread. For example,
		if you're calling this from your window thread, you won't
		get any Draw updates in your window's views until this
		returns, which might be a while if the UI of the ScannerBe
		add-on is a non-modal window.

	GetNextScannerImage()

		If you don't mind doing a bit more work yourself, by calling
		scan_open(), scan_start(), and scan_close() yourself, then
		you can arrange things to do multiple scans, retrieving
		multiple images in one session.

		Basically:

			BBitmap *bitmap;
			scan_id scanID;
			scan_open( NULL, &scanID );
			while( keep_scanning_flag && stat == B_OK ) {
				scan_start( scanID );
				bitmap = GetNextScannerImage( scanID, status )
				app_process_image( bitmap );
			scan_close( scanID );

		Note that scan_start() blocks until the user clicks the scan
		button, so if you are using a ScannerBe add-on that doesn't
		have an interface (for example, if you're just batch scanning
		with an ADF) you can pull scan_start() out of the loop, or
		remove it altogether, depending on your purpose.
*/

#include "ScanGlue.h"
#include <Bitmap.h>

static scan_id gScanID = 0;

BBitmap* GetScannerImage( status_t &status )
{
	scan_version version;
	status = scan_open( NULL, &gScanID, &version );
	if( status != B_OK )
		return NULL;
	
	status = scan_start( gScanID );
	if( status != B_OK )
		return NULL;
	
	BBitmap *bitmap = GetNextScannerImage( gScanID, status );
	
	status_t closeStatus = scan_close( gScanID );
	
	if( status != B_OK )
		return NULL;
	
	if( closeStatus != B_OK ) {
		if( bitmap )
			delete bitmap;
		status = closeStatus;
		return NULL;
	}
	
	gScanID = 0;
	return bitmap;
}

BBitmap* GetNextScannerImage( scan_id id, status_t &status  )
{
	const int32 kBufSize = 32 * 1024L;
	char *scanBuf = new char[ kBufSize ];
	BBitmap *bitmap = NULL;
	
	status = scan_open_image( id );
	if( status != B_OK )
		return NULL;
	
	// When scan_start() returns, the user has done their preview fiddled with
	// the settings, and then clicked the scan button. The scanner may go ahead
	// and do the scan, or it may wait until the first time we ask for the image
	// in scan_data() below. Always check for what's coming back here.
	
	scan_settings settings;
	status = scan_get_settings( id, &settings, NULL, NULL );
	if( status != B_OK )
		goto errXit;
	
	// This demo app only handles gray & color images, one byte per sample,
	// but this is where you'd handle the threshold case and possibly handle
	// deeper than 24-bit color, etc.
	if( ( settings.image_type != SCAN_TYPE_RGB ) &&
			( settings.image_type != SCAN_TYPE_RGB ) ) {
		goto errXit;
	}
	
	// Make the bitmap that we're going to draw.
	BRect bitmapRect(  0, 0,
			settings.pixel_width - 1, settings.pixel_height - 1 );
	color_space space = B_RGB_32_BIT;
	bitmap = new BBitmap( bitmapRect, space ) ;
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
					*dest++ = *ptr;		// B
					*dest++ = *ptr;		// G
					*dest++ = *ptr;		// R
					*dest++ = 0;		// A
				}
				offset += count * 4;
			}
			// still gotta do thresholded data
		}
		
		// You might take this code and do other things in the scan loop,
		// like check for user cancellation, etc.
	}
	
errXit:
	status_t closeStatus = scan_close_image( id );
	delete[] scanBuf;
	if( ( closeStatus != B_OK ) || ( status != B_OK ) ) {
		if( bitmap )
			delete bitmap;
		return NULL;
	}
	return bitmap;
}

