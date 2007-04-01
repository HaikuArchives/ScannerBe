// ScannerBe Add-on for Becasso, Copyright © 1997, Jim Moy

#include "BecassoAddOn.h"
#include "ScanGlue.h"
#include <Application.h>
#include <string.h>
#include <stdio.h>

#pragma export on

const uint32 kTriggerScan = 'trig';

// The interesting part about this add-on is that Becasso wants to be
// informed of when an image is ready; it doesn't call and wait for
// an image like is most natural in the ScannerBe interface. To work
// in this manner, we put the ScannerBe add-on in a perpetual state of
// readiness, i.e., as soon as an image is delivered by the add-on
// we immediately call scan_start() again, waiting for the indication
// that we can get the image. The ClickWatcher class handles this
// interaction.
class ClickWatcher : public BLooper {
public:
virtual void	MessageReceived( BMessage *msg );
};

static bool				gBeScanOpen = false;
static bool				gScanStartFailed = false;
static scan_id			gScanID = 0;
static ClickWatcher		*gClickWatch = NULL;
static uint32			gIndex;

void do_close();

int addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "Scanner");
	strcpy (info->author, "Jim Moy");
	strcpy (info->copyright, "© 1998 Jim Moy");
	strcpy (info->description, "Add-on to scan an image using ScannerBe");
	info->type				= BECASSO_CAPTURE;
	info->index				= index;
	info->version			= 1;
	info->release			= 0;
	info->becasso_version	= 1;
	info->becasso_release	= 0;
	info->does_preview		= true;
	gIndex = index;
	gScanStartFailed = false;
	gClickWatch = new ClickWatcher;
	gClickWatch->Run();
	return (0);
}

void do_close()
{
	status_t status = B_OK;
	
	// Break us out of any pending scan_start() call. If the ScannerBe
	// add-on decides to quit and return an error from scan_start(), e.g.,
	// if a cancel button on the UI is clicked, then scan_start() will
	// have returned an error, and we don't need to cancel it. If we
	// are still waiting on scan_start() in the ScannerBe add-on, and
	// want to shutdown the Becasso add-on (when Becasso quits) then
	// we have to shut it down here to allow it to clean up.
	if( gBeScanOpen && ( ! gScanStartFailed ) ) {
		status = scan_close_image( gScanID );
	}
	
	if( gBeScanOpen && ( gScanID != 0 ) ) {
		status = scan_close( gScanID );
		gBeScanOpen = false;
		gScanID = 0;
	}
	
	if( gClickWatch ) {
		gClickWatch->PostMessage( B_QUIT_REQUESTED );
		gClickWatch = NULL;
	}
}

int addon_exit (void)
{
	do_close();
	return (0);
}

int addon_open (BWindow *, const char *)
{
	status_t status = B_OK;
	
	scan_version version;
	if( ! gBeScanOpen ) {
		status = scan_open( NULL, &gScanID, &version );
		if( status == B_OK )
			gBeScanOpen = true;
	}
	
	if( status != B_OK )
		return B_ERROR;

	gClickWatch->PostMessage( kTriggerScan );
		
	return B_OK;
}

int addon_close ()
{
	do_close();
	return (0);
}

BBitmap *bitmap (char *title)
{
	strcpy (title, "Scanned Image");
	status_t status = B_OK;
	
	// get an image from the ScannerBe add-on
	BBitmap *bitmap = GetNextScannerImage( gScanID, status );
	
	// get ready for another scan
	gClickWatch->PostMessage( kTriggerScan );

	return bitmap;
}

#pragma export off

void ClickWatcher::MessageReceived( BMessage *msg )
{
	if( msg->what != kTriggerScan ) {
		BLooper::MessageReceived( msg );
		return;
	}

	status_t status = B_OK;
	status = scan_start( gScanID );
	if( status == B_OK ) {
		BMessage msg( CAPTURE_READY );
		status = msg.AddInt32( "index", gIndex );
		if( status == B_OK )
			be_app->PostMessage( &msg );
	} else {
		// Failed scan_start(), which in this case the UI is
		// no longer ready to start a scan, so indicate that
		// to addon_exit() that we don't need to cancel out of
		// a pending scan_start().
		gScanStartFailed = true;
	}
}

