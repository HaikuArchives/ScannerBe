/*
	ScannerBe -- Scanner interface for BeOS. 2/2/98
	Copyright (c) 1997, Jim Moy, All Rights Reserved
*/

#ifndef _SCANNERBE_H
#define _SCANNERBE_H

#include <Errors.h>
#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version of the library, compared on scan_open() and will return
	error code if the shared lib you're using isn't at least this high. */
const int32		kScanLibVersionMajor = 0;
const int32		kScanLibVersionMinor = 9;
const int32		kScanLibVersionIncr = 0;

/* status_t result codes, in addition to Errors.h values */
const status_t SCAN_ERROR_BASE	= B_GENERAL_ERROR_BASE + 0xc000;

enum {
		/* normal results */
	SCAN_DATA_END = SCAN_ERROR_BASE,	/* all scan data read */
		/* error results */
	SCAN_ERROR,
	SCAN_BAD_PARAM,						/* wrong value for argument */
	SCAN_BAD_PHASE,						/* made a call in the wrong order */
	SCAN_BAD_CONFIG,
	SCAN_NO_ADDON,						/* no add-on containing req'd scanner */
	SCAN_ADDON_ERROR,					/* add-on error */
	SCAN_NO_SCANNER,					/* add-on couldn't find the scanner */
	SCAN_USER_CANCEL,					/* cancel button on progress indicator */
	SCAN_BADID,							/* passed a bad scan_id */
	SCAN_INVALID_SETTING				/* scanner rejected scan_settings field(s) */
};

/* Scanner image types */
typedef uint32 scan_type;
const scan_type SCAN_TYPE_BINARY	= 1;	/* one bit per pixel, rows on byte bounds */
const scan_type SCAN_TYPE_GRAY		= 2;	/* grayscale, one sample per pixel */
const scan_type SCAN_TYPE_RGB		= 4;	/* RGB, three samples per pixel, no alpha */
const scan_type SCAN_TYPE_UNKNOWN	= 8;	/* scanner set to unknown data type */

/* Version of libscanbe */
const short SCAN_STRING_LENGTH = 256;
typedef struct {
	int32	major;
	int32	minor;
	int32	incr;
	char	info[SCAN_STRING_LENGTH];
} scan_version;

/* 300dpi coordinates, for specifying an area on the scanner bed */
typedef struct {
	int32	left;
	int32	top;
	int32	right;
	int32	bottom;
} scan_rect;

/* Scanner settings struct, for convenience using the all-in-one functions
	for get/put. The types in this structure also specify the type that
	must be passed to the one-setting put/get functions. */
typedef struct {
	scan_rect	scan_area;		/* area on scanner bed to scan */
	scan_type	image_type;		/* mask or value, see docs */
	uint32		pixel_bits;		/* bits per pixel */
	uint32		resolution;		/* dpi */
	int32		brightness;		/* arbitrary, scanner-specific values */
	int32		contrast;		/* arbitrary, scanner-specific values */
	int32		scaling;		/* in percent */
		/* These only come back in the "current" settings,
			and don't have any effect with scan_put_settings(). */
	uint32		pixel_width;	/* as result of scan_area and resolution */
	uint32		pixel_height;	/* as result of scan_area and resolution */
	uint32		row_bytes;		/* accounts for padding */
} scan_settings;

/* Have to look up the field in scan_settings corresponding to the
	scan_setting_id below to find out which is the right field in
	the union to use. The ptr field is only for SCAN_SETTING_SPECIFIC
	for scanner-specific stuff. */
typedef union {
	scan_rect	rect;
	scan_type	type;
	uint32		u_int;
	int32		s_int;
	void*		ptr;
} scan_value;

typedef uint32 scan_setting_id;
const scan_setting_id	SCAN_SETTING_AREA			= 1;
const scan_setting_id	SCAN_SETTING_IMAGETYPE		= 2;
const scan_setting_id	SCAN_SETTING_PIXELBITS		= 4;
const scan_setting_id	SCAN_SETTING_RESOLUTION		= 8;
const scan_setting_id	SCAN_SETTING_BRIGHTNESS		= 16;
const scan_setting_id	SCAN_SETTING_CONTRAST		= 32;
const scan_setting_id	SCAN_SETTING_SCALING		= 64;
const scan_setting_id	SCAN_SETTING_WIDTH			= 128;
const scan_setting_id	SCAN_SETTING_HEIGHT			= 256;
const scan_setting_id	SCAN_SETTING_ROWBYTES		= 512;
const scan_setting_id	SCAN_SETTING_TONEMAP		= 1024;
const scan_setting_id	SCAN_SETTING_TONEMAP3		= 2048;
const scan_setting_id	SCAN_SETTING_SPECIFIC		= 4096;

/* Mask to specify a set of settings. */
typedef uint32 scan_settings_mask;

/* Specifies which kind of value you want from scan_get_one_setting(). */
typedef uint32 scan_setting_kind;
const scan_setting_kind		SCAN_SETTING_CURRENT		= 1;
const scan_setting_kind		SCAN_SETTING_MINIMUM		= 2;
const scan_setting_kind		SCAN_SETTING_MAXIMUM		= 3;
	
/* Reference number for scan session */
typedef void*	scan_id;

/* Callback for scan_get_addons(). */
typedef uint32 scan_addon_flag;
const scan_addon_flag		SCAN_FLAG_UI				= 1;
const scan_addon_flag		SCAN_FLAG_DRIVER			= 2;
typedef struct {
	char				*path;
	scan_addon_flag		flags;
	char				info[SCAN_STRING_LENGTH];
} CallbackInfo;
typedef status_t (*ScanAddonProc)( const CallbackInfo *info, void *data );

/* libscanbe interface */
void		scan_get_version( scan_version *version );
status_t	scan_get_addons( ScanAddonProc callback, void *data );
status_t	scan_open( const char* name, scan_id *id, scan_version *version );
status_t	scan_close( const scan_id id );
status_t	scan_get_capabilities( const scan_id id, scan_settings_mask *mask );
status_t	scan_get_settings( const scan_id id, scan_settings *current,
								scan_settings *minimum, scan_settings *maximum );
status_t	scan_put_settings( const scan_id id, scan_settings *settings,
								scan_settings_mask *mask );
status_t	scan_get_one_setting( const scan_id id,
								const scan_setting_id setting,
								const scan_setting_kind setting_kind,
								scan_value *value_ptr );
status_t	scan_put_one_setting( const scan_id id,
								const scan_setting_id setting,
								scan_value *value_ptr, scan_settings_mask *mask );
status_t	scan_start( const scan_id id );
status_t	scan_open_image( const scan_id id );
status_t	scan_close_image( const scan_id id );
status_t	scan_data( const scan_id id, void *buffer, int32 *count );
bool		scan_adf_ready( const scan_id id );
void		scan_error_message( const scan_id id, status_t error_id, char *msg );

#ifdef __cplusplus
}
#endif

#endif  // _SCANNERBE_

