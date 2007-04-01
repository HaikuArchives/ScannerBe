/*
	ScannerBe Sample Code -- Copyright (c) 1997, Jim Moy, All Rights Reserved
	
	This is a skeleton of a ScannerBe add-on. You can start here and fill things
	in as you need them. This is essentially my HP add-on with its guts ripped
	out, so you know at least the skeleton works.
*/

#pragma export on
#include "ScanAddOn.h"
#pragma export off
#include "ScanAddOnProto.h"
#include <stdlib.h>

#define kVersionMajor	0
#define kVersionMinor	9
#define kVersionIncr	0

/* In this skeleton, I'm just using this cookie to figure out when
	my whole image has been transferred. You can use it for whatever
	you want. */
typedef struct {
	uint32			first_time_flag;
	uint32			total_bytes;
	uint32			byte_count;
	uint32			row_bytes;
} my_cookie;

static const char *my_scanner_name[] = {
	"application/x-vnd.jbm-myscanner",
	"ScannerBe Sample Add-on  (Jim Moy v0.9)",
	NULL
};

static scan_hooks my_scanner_hooks = {
	scn_open,
	scn_close,
	scn_get_capabilities,
	scn_get_setting,
	scn_put_setting,
	scn_open_image,
	scn_close_image,
	scn_start,
	scn_data,
	scn_adf_ready,
	scn_error_message
};

const char **
publish_scanners()
{
	return my_scanner_name;
}

scan_hooks *
find_scanner(const char *name)
{
	return &my_scanner_hooks;
}

static status_t scn_open( scan_version *version, void  **cookie )
{
	status_t status = B_OK;
	my_cookie *goodie = (my_cookie *) malloc( sizeof( my_cookie ) );

	// Reset the scanner, initialize things, stash a cookie.
			
	goodie->first_time_flag = TRUE;
	goodie->total_bytes = 0;
	goodie->byte_count = 0;
	*cookie = goodie;
	version->major = kVersionMajor;
	version->minor = kVersionMinor;
	version->incr = kVersionIncr;
	return B_OK;
}

static status_t scn_close( void *cookie )
{
	my_cookie *goodie = (my_cookie *) cookie;
	free( goodie );
	return B_OK;
}

static status_t scn_get_capabilities( void *cookie, scan_settings_mask *mask )
{
	// These are typical for a flatbed scanner. You may actually have to query
	// your scanner to see what is supported, if it supports more than one
	// model.
	*mask =
		SCAN_SETTING_AREA				|
		SCAN_SETTING_IMAGETYPE		|
		SCAN_SETTING_RESOLUTION		|
		SCAN_SETTING_BRIGHTNESS		|
		SCAN_SETTING_CONTRAST			|
		SCAN_SETTING_SCALING
	;
	
	return B_OK;
}

static status_t scn_get_setting(
	void						*cookie,
	const scan_setting_id		setting_id, 
	const scan_setting_kind		setting_kind,
	scan_value					*value_ptr )
{
	my_cookie *goodie = (my_cookie *) cookie;
	
	switch( setting_kind ) {
	case SCAN_SETTING_CURRENT:
		break;
	case SCAN_SETTING_MINIMUM:
		break;
	case SCAN_SETTING_MAXIMUM:
		break;
	}
	
	switch( setting_id ) {
	
	case SCAN_SETTING_AREA:
		break;
		
	case SCAN_SETTING_IMAGETYPE:
		break;
		
	case SCAN_SETTING_PIXELBITS:
		break;
		
	case SCAN_SETTING_RESOLUTION:
		break;
		
	case SCAN_SETTING_BRIGHTNESS:
		break;
		
	case SCAN_SETTING_CONTRAST:
		break;
		
	case SCAN_SETTING_SCALING:
		break;
		
	case SCAN_SETTING_WIDTH:
		break;
		
	case SCAN_SETTING_HEIGHT:
		break;
		
	case SCAN_SETTING_ROWBYTES:
		break;
		
	}
	
	return B_OK;
}

static status_t scn_put_setting(
	void					*cookie,
	const scan_setting_id	setting_id, 
	scan_value				*value_ptr,
	scan_settings_mask		*settings_mask )
{
	status_t status = B_OK;
	my_cookie *goodie = (my_cookie *) cookie;
	*settings_mask = 0;
	
	switch( setting_id ) {
	
	case SCAN_SETTING_AREA:
		break;
		
	case SCAN_SETTING_IMAGETYPE:
		break;
		
	case SCAN_SETTING_PIXELBITS:
		break;
		
	case SCAN_SETTING_RESOLUTION:
		break;
		
	case SCAN_SETTING_BRIGHTNESS:
		break;
		
	case SCAN_SETTING_CONTRAST:
		break;
		
	case SCAN_SETTING_SCALING:
		break;
		
	case SCAN_SETTING_WIDTH:
	case SCAN_SETTING_HEIGHT:
	case SCAN_SETTING_ROWBYTES:
	default:
		return SCAN_INVALID_SETTING;
	}
	
	return B_OK;
}

static status_t scn_open_image( void *cookie )
{
	my_cookie *goodie = (my_cookie *) cookie;
	
	goodie->first_time_flag = TRUE;
	
	goodie->row_bytes = 0;		// bytes per row
	goodie->total_bytes = 0;	// total bytes for this image
	goodie->byte_count = 0;
	
	return B_OK;
}

static status_t scn_close_image( void *cookie )
{
	return B_OK;
}

static status_t scn_start( void *cookie )
{
	return B_OK;
}

static status_t scn_data( void *cookie , void* buffer, int32* count )
{
	my_cookie *goodie = (my_cookie *) cookie;
	int32 tmpCount, maxRows, realCount;
	/* Figure out in scn_start() how many bytes per row, then this can
		calculate what the number of whole rows is. */
	maxRows = *count / goodie->row_bytes;
	realCount = maxRows * goodie->row_bytes;
	if( goodie->first_time_flag ) {
		goodie->first_time_flag = FALSE;
	}
	
	/* Do the scan here. */

	if( count == 0 )
		return SCAN_DATA_END;
	else {
		/* Here's one way of figuring out when you're done with
			your scan. I'm sure there are many others, depending
			on the device. */
		tmpCount = goodie->byte_count + *count;
		if( tmpCount >= goodie->total_bytes ) {
			*count = goodie->total_bytes - goodie->byte_count;
			return SCAN_DATA_END;
		} else {
			goodie->byte_count += *count;
			return B_OK;
		}
	}
	
	return SCAN_ERROR;
}

bool scn_adf_ready( void *cookie )
{
	return false;
}

void scn_error_message( void *cookie, status_t err, char *msg )
{
	msg[0] = 0;
}


