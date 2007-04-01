/* ScannerBe sample code. Copyright © Jim Moy, 1997, All rights reserved. */

#include "ScannerBe.h"
#include "Datatypes.h"
#include <Application.h>
#include <Directory.h>
#include <Errors.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>
#include <stdlib.h>

// some buffers for image data
const int32 kBufSize = 16 * 1024L;
char gScanBuf[ kBufSize ];
char gRowBuf[ kBufSize ];
scan_id gScanID;

const char *kSig = "application/x-vnd.jbm-scandemo";

// simple app just to demo a scan

class ScanApp : public BApplication {
public:
					ScanApp() : BApplication( kSig ) { Run(); }
virtual void		ReadyToRun();
		void		do_scan();
		status_t	write_buffer( FILE*, void*, int32, scan_settings&, DATABitmap& );
};

void main() { ScanApp app; }

// Do the setup and teardown here, so even if there's a failure down
// in do_scan(), the libraries always get shut down anyway.

void ScanApp::ReadyToRun()
{
	if( DATAInit( kSig ) != B_OK ) {
		fprintf( stderr, "Couldn't init Datatypes\n" );
		return;
	}
	
	// Before doing the scan_open() call, you might want to do some version
	// checking with scan_get_version(). Also, if you're doing to be
	// selecting a scanner with no user interaction, you should probably
	// check the valid ones with scan_get_addons() and see if it meets your
	// specs.
	scan_version version;
	const char *scannerName = "application/x-vnd.jbm-hpscanjet";
	printf( "opening scanner: %s\n", scannerName );
	status_t status = scan_open( scannerName, &gScanID, &version );
	if( status != B_OK ) {
		DATAShutdown();
		fprintf( stderr, "Couldn't open scanner: %s\n", scannerName );
		return;
	}

	do_scan(); // all the work is in here
	
	status = scan_close( gScanID );
	if( status != B_OK )
		fprintf( stderr, "Did not complete the scan (0x%X)\n", status );
	
	DATAShutdown();
	PostMessage( B_QUIT_REQUESTED );
}

void ScanApp::do_scan()
{
	status_t status = B_OK;
	
	printf( "getting scan session settings\n" );
	// Since I'm only going to be changing a couple of settings for this
	// demo, I'm going to ask the scanner for the current settings, then
	// just modifiy a few before sending it back.
	scan_settings settings;
	status = scan_get_settings( gScanID, &settings, NULL, NULL );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't get scanner settings (0x%X)\n", status );
		return;
	}
	
	printf( "putting back modified settings\n" );
	// For this demo, use the default full-page scan, but make
	// it low resolution and color.
	settings.resolution = 72;
	settings.image_type = SCAN_TYPE_RGB;
	settings.pixel_bits = 24;
	scan_settings_mask mask;
	status = scan_put_settings( gScanID, &settings, &mask );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't put scanner settings (0x%X)\n", status );
		return;
	}
	
	// Note that changing the resolution and image type changed various
	// other things, as reflected in mask. These calls demo how to get one
	// setting at a time out of the scanner, but in this case I probably would've
	// just called scan_get_settings() again, since there's nothing performance
	// critical. However, these kinds of calls are good if you're doing something
	// like dragging a scaling slider around on an interface and want a reasonable
	// update rate on some indicators while talking to some, uh, low-cost processor
	// inside a scanner.
	printf( "querying image size\n" );
	scan_value value;
	status = scan_get_one_setting( gScanID, SCAN_SETTING_WIDTH,
							SCAN_SETTING_CURRENT, &value );
	settings.pixel_width = value.u_int;
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't read new pixel width from scanner (0x%X)\n", status );
		return;
	}
	status = scan_get_one_setting( gScanID, SCAN_SETTING_HEIGHT,
							SCAN_SETTING_CURRENT, &value );
	settings.pixel_height = value.u_int;
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't read new pixel height from scanner (0x%X)\n", status );
		return;
	}
	status = scan_get_one_setting( gScanID, SCAN_SETTING_ROWBYTES,
							SCAN_SETTING_CURRENT, &value );
	settings.row_bytes = value.u_int;
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't read new pixel height from scanner (0x%X)\n", status );
		return;
	}

	// Set up the Datatypes header.
	DATABitmap dmap;
	dmap.magic = DATA_BITMAP;
	dmap.bounds.Set( 0, 0,
		settings.pixel_width -1, settings.pixel_height - 1 );
	dmap.colors = B_RGB_32_BIT;
	dmap.rowBytes = settings.row_bytes * 4 / 3;
	int32 width = dmap.bounds.IntegerWidth();
	dmap.dataSize = ( dmap.bounds.IntegerHeight() + 1 ) * dmap.rowBytes;
	
	// We'll write it to a temporary file, then hand it off to the
	// Datatypes library to convert the image.
	char tmpPath[256];
	tmpnam( tmpPath );
	FILE *fp = fopen( tmpPath, "w+b" );
	if( ! fp ) {
		fprintf( stderr, "Couldn't open the temporary file\n", status );
		return;
	}
	
	// Datatypes header.
	int32 count = fwrite( &dmap, 1, sizeof( DATABitmap ), fp );
	if( count <= 0 ) {
		fprintf( stderr, "Couldn't write the DATABitmap header (0x%X)\n", status );
		fclose( fp );
		return;
	}

	printf( "writing image to file\n" );
	// Since we're doing this "non-interactive," we don't have to call
	// scan_start() and can just blow by it and proceed directly to scanning.

	status = scan_open_image( gScanID );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't open the image (0x%X)\n", status );
		return;
	}
	
	// Main scanning loop. Get a buffer at a time and do the padding
	// conversion required by Datatypes.
	for( bool notDone = true; notDone; ) {
		count = kBufSize;
		
		// scan data into gScanBuf, get the number of bytes scanned back in count
		status = scan_data( gScanID, gScanBuf, &count );
		
		if( status != B_OK ) {
			// if we've reached the end of the scan, we get back SCANNER_DATA_END
			if( status == SCAN_DATA_END )
				status = B_OK;
			else				
				scan_close_image( gScanID );			// some other scanner error
			notDone = false;
		}
		
		if( status == B_OK && count > 0 ) {
			// As required by scan_data(), this will always come out to
			// be an integral number of lines.
			int32 lines = count / settings.row_bytes;
			// Write a buffer at a time out to the tmp file.
			if( write_buffer( fp, gScanBuf, lines, settings, dmap ) != B_OK ) {
				notDone = false;
				scan_close_image( gScanID );
			}
		}
		
		// Do other things in the scan loop, like check for user
		// cancellation of the scan, etc.
	}

	status = scan_close_image( gScanID );
	if( status != B_OK ) {
		fprintf( stderr, "Couldn't close the image (0x%X)\n", status );
		return;
	}
	
	// We're all done scanning!
	fclose( fp );
	
	// Now do the Datatypes conversion,
	// just drop the file in the app's directory.
	BFile inFile( tmpPath, B_READ_ONLY );
	status_t ioErr = inFile.InitCheck();
	if( ioErr != B_OK ) {
		fprintf( stderr, "Couldn't open %s\n", tmpPath );
		remove( tmpPath );
		return;
	}
	app_info info;
	status = be_roster->GetAppInfo( kSig, &info );
	BEntry entry( &info.ref );
	BDirectory dir;
	ioErr = entry.GetParent( &dir );
	if( ( status != B_OK ) || ( entry.InitCheck() != B_OK ) || ( ioErr != B_OK ) ) {
		fprintf( stderr, "Couldn't get app directory\n" );
		return;
	}
	const char *fileName = "scanned_image.tga";
	BFile outFile( &dir, fileName, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE );
	ioErr = outFile.InitCheck();
	if( ioErr != B_OK ) {
		BPath p( &dir, fileName );
		fprintf( stderr, "Couldn't open %s\n", p.Path() );
		remove( tmpPath );
		return;
	}
	printf( "doing Datatypes conversion\n" );
	status = DATATranslate( inFile, NULL, NULL, outFile, 'TGA ' );
	if( status != B_OK )
		fprintf( stderr, "DATATranslate() failed (0x%X)\n", status );
	
	// There. Now we have a nice, plain RGB image with linear data from
	// the scanner at whatever default tonemap it had setup inside it.
	// Should be able to just open it and RRaster will use the same
	// translator to show it to you. Clean up the tmp file that had
	// DATABitmap data in it.
	remove( tmpPath );
	printf( "done\n" );
}

// Pad the RGB data so it'll stream into Datatypes.

status_t ScanApp::write_buffer( FILE *fp, void *buf, int32 lines,
								scan_settings &settings, DATABitmap &dmap )
{
	status_t status = B_OK;
	int32 writeCount = 0;
	char *rowPtr = (char *) buf;
	char *ptr = rowPtr;
	char *dest = NULL;
	
	for( int32 i = 0; ( i < lines ) && ( status == B_OK ); i++ ) {
		dest = gRowBuf;
		// Datatypes manual disagrees with BGRA format, bit BBitmap doc is right.
		for( int32 j = 0; j < settings.pixel_width; j++ ) {
			// (make your processing loop faster than this one...)
			*dest++ = *(ptr+2);
			*dest++ = *(ptr+1);
			*dest++ = *ptr;
			*dest++ = 0;
			ptr += 3;
		}
		writeCount = fwrite( gRowBuf, 1, dmap.rowBytes, fp );
		if( writeCount <= 0 )
			status = B_FILE_ERROR;
		// next row
		rowPtr += settings.row_bytes;
		ptr = rowPtr;
	}
	return status;
}

