/*
	ScannerBe -- Scanner interface for BeOS. 2/2/98
	Copyright (c) 1997, Jim Moy, All Rights Reserved
*/

#ifndef _LIBSCANBE_H
#define _LIBSCANBE_H

#pragma export on
#include "ScannerBe.h"
#pragma export off

#include "ScanAddOn.h"
#include "ScanBeConst.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <Path.h>
#include <Resources.h>
#define DEBUG 1
#include <Debug.h>
#include "Preferences.h"

#include <image.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*	This is just a dummy placeholder, so I can identify my library's
	file among others an app may be using so I can find resources. */
#pragma export on
int32 libscanbe_identifier = 0;
const char*			kMyIdentifierName		= "libscanbe_identifier";
#pragma export off

/*	Constants. */
const char*			kAddonMimeType			= "application/x-be-executable";
const char*			kTypeAttr				= "BEOS:TYPE";
const type_code		kStringType				= 'cstr';
const int32			kSubdirNameID			= 0;

/*	Enforce some ordering of the calls. */
typedef enum {
	kScanStateClosed,
	kScanStateOpen,
	kScanStateImageOpen,
	kScanStateData
} scan_state;

/* Place to save the user-selected scanner. */
Preferences gPrefs( "x-vnd.jbm-libscanbe" );
PreferenceSet gSettings( gPrefs, "settings", true );
const char* kPrefScannerName = "current_scanner";

/* List to keep track of valid scan session identifiers. */
class scanner_entry {
public:
	scanner_entry() { image = 0; hooks = NULL, cookie = NULL;
						state = kScanStateClosed; }
	image_id		image;
	scan_hooks*		hooks;
	void*			cookie;
	scan_state		state;
};
static BList gScannerList;
static BLocker gListLocker;

/* For passing data to a callback function. */
struct walk_info {
	char		*name;
	image_id	id;
};

/* Troubleshooting. */
static bool gDebug = false;
const char *dbgname = "libscanbe";

/*	These are for querying the image of the add-on. */
typedef scan_hooks* (*find_proc)( const char *name );

/*	Some local function prototypes. */
static status_t get_settings( scanner_entry *entry, scan_setting_kind kind,
								scan_settings *settings );
static status_t put_settings( scanner_entry *entry,
								scan_settings *settings,
								scan_settings_mask *mask );
static status_t check_addon_name( const CallbackInfo *info, void *data );
static status_t get_addon_dir( BDirectory* dir );
static status_t get_addon_info( const BEntry &entry, CallbackInfo &info );

/* Used by the prefs applet, but not officially part of the API. */
#pragma export on
extern "C" bool libscanbe_get_scanner( char *name );
extern "C" bool libscanbe_save_scanner( const char *name );
#pragma export reset

#pragma mark ---- Public Functions ----

void scan_get_version( scan_version *version )
{
	version->major = kScanLibVersionMajor;
	version->minor = kScanLibVersionMinor;
	version->incr = kScanLibVersionIncr;
	sprintf( version->info, "libscanbe, v%ld.%ld.%ld",
			kScanLibVersionMajor, kScanLibVersionMinor, kScanLibVersionIncr );
}

status_t scan_get_addons( ScanAddonProc callback, void *data )
{
	if( ! callback )
		return SCAN_BAD_PARAM;
		
	BEntry entry;
	BNode node;
	int32 len = 0;
	bool keepGoing = true;
	status_t status;
									// grab add-on subdir
	BDirectory dir;
	status = get_addon_dir( &dir );
	if( status != B_OK ) {
		if( gDebug )
			printf( "%s: couldn't find add-on directory\n", dbgname );
		return status;
	}
	dir.Rewind();
									// add-ons can be symlinks
	while( keepGoing ) {
		status = dir.GetNextEntry( &entry );
		if( status != B_OK ) {
			keepGoing = false;
			continue;
		}
		if( entry.IsSymLink() ) {	// resolve sym-link
			entry_ref ref;
			status = entry.GetRef( &ref );
			if( status != B_OK )
				continue;
			status = entry.SetTo( &ref, true );
			if( status != B_OK )
				continue;
		}
		if( ! entry.IsFile() )
			continue;
		if( B_OK != node.SetTo( &entry ) )
			continue;
									// check for right file type
		char buf[256];
		len = node.ReadAttr( kTypeAttr, 0, 0, buf, 255 );
		if( len <= 0 )
			continue;
		buf[len] = 0;
		if( strcmp( buf, kAddonMimeType ) )
			continue;
									// whoever wanted the info, here it is
		CallbackInfo info;
		status = get_addon_info( entry, info );
		if( status == B_OK ) {
			status = callback( &info, data );
			if( status == 0 )
				continue;
			else
				return status;
		}
		if( info.path )
			delete[] info.path;
	}
	
	return B_OK;
}

/* Loads the scanner add-on and call its open() function. */
status_t scan_open( const char* name, scan_id *id, scan_version *version )
{
	status_t status = B_OK;
	char *debugEnviron = getenv( "SCAN_DEBUG" );
	gDebug = debugEnviron && strlen( debugEnviron ) > 0;
	if( gDebug )
		printf( "%s: entering scan_open\n", dbgname );
		
								// locate the add-on dir
	BDirectory dir;
	status = get_addon_dir( &dir );
	if( status != B_OK ) {
		if( gDebug )
			printf( "%s: couldn't find add-on directory\n", dbgname );
		return status;
	}
								// none specified, take the prefs applet one
	if( ! name ) {
		char buf[SCAN_STRING_LENGTH];
		if( libscanbe_get_scanner( buf ) )
			name = buf;
		else {
			if( gDebug )
				printf( "%s: no scanner selected in prefs applet\n", dbgname );
			return SCAN_NO_ADDON;
		}
	}
	
	walk_info info;
	info.name = (char *) name;
	status = scan_get_addons( check_addon_name, &info );
	if( status != B_OK || info.id < B_OK )
		return SCAN_NO_ADDON;
								// make the scanner entry
	find_proc func;
	status = get_image_symbol( info.id, kFindScannerFunction,
								B_SYMBOL_TYPE_TEXT, &func );
	if( status != B_OK ) {
		if( gDebug )
			printf( "%s: no %s function in add-on\n", dbgname, kFindScannerFunction );
		goto errXit;
	}
	
	scan_hooks* hooks = func( name );
	if( ! hooks ) {
		if( gDebug )
			printf( "%s: no add-on hooks for %s\n", dbgname, name );
		return B_BAD_ADDRESS;
	}
	
	scanner_entry *entry = new scanner_entry;
	ASSERT( entry );
	entry->image = info.id;
	entry->hooks = hooks;
	gListLocker.Lock();
	gScannerList.AddItem( entry );
	gListLocker.Unlock();
	*id = entry;

	status = entry->hooks->open( version, &entry->cookie );
	if( status == B_OK )
		entry->state = kScanStateOpen;
	else {
		if( gDebug )
			printf( "%s: open hook failed: %ld\n", status );
	}
	return status;
	
errXit:
	unload_add_on( info.id );
	return status;
}


status_t scan_close( const scan_id id )
{
	if( gDebug )
		printf( "%s: entering scan_close\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
	
	scanner_entry *entry = (scanner_entry *) id;
	
	if( entry->state < kScanStateOpen ) {
		if( gDebug )
			printf( "%s: id not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}
	
	status_t status = B_OK;
	if( entry->state >= kScanStateImageOpen ) {
		status = entry->hooks->close_image( entry->cookie );
		if( status != B_OK )
			return status;
	}
	
	gListLocker.Lock();
	gScannerList.RemoveItem( entry );
	gListLocker.Unlock();

	status = entry->hooks->close( entry->cookie );
	if( status != B_OK ) {
		if( gDebug )
			printf( "%s: close hook failed: %ld\n", dbgname, status );
		return status;
	}
	
	return B_OK;
}


status_t scan_get_capabilities( const scan_id id , scan_settings_mask *mask )
{
	if( gDebug )
		printf( "%s: entering scan_get_capabilities\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen ) {
		if( gDebug )
			printf( "%s: id not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}

	status_t status = entry->hooks->get_capabilities( entry->cookie, mask );
	if( status != B_OK && gDebug )
		printf( "%s: get_capabilities hook failed: %d\n", dbgname, status );
		
	return status;
}

status_t scan_get_settings( const scan_id id, scan_settings *current,
						scan_settings *minimum, scan_settings *maximum )
{
	if( gDebug )
		printf( "%s: entering scan_get_settings\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen ) {
		if( gDebug )
			printf( "%s: id not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}

	status_t status = B_OK;
	
	if( current ) {
		status = get_settings( entry, SCAN_SETTING_CURRENT, current );
		if( status != B_OK )
			return status;
	}
	if( minimum ) {
		status = get_settings( entry, SCAN_SETTING_MINIMUM, minimum );
		if( status != B_OK )
			return status;
	}
	if( maximum ) {
		status = get_settings( entry, SCAN_SETTING_MAXIMUM, maximum );
		if( status != B_OK )
			return status;
	}
	
	return B_OK;
}


status_t scan_put_settings( const scan_id id, scan_settings *settings,
							scan_settings_mask *mask )
{
	if( gDebug )
		printf( "%s: entering scan_put_settings\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen || entry->state >= kScanStateImageOpen ) {
		if( gDebug )
			printf( "%s: id not open, or image already open\n", dbgname );
		return SCAN_BAD_PHASE;
	}

	status_t status = put_settings( entry, settings, mask );
	return status;
}


status_t scan_get_one_setting( const scan_id id, const scan_setting_id setting,
								const scan_setting_kind setting_kind,
								scan_value *value_ptr )
{
	if( gDebug )
		printf( "%s: entering scan_get_one_setting (%ld)\n", dbgname, setting );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen ) {
		if( gDebug )
			printf( "%s: id not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}

	status_t status = entry->hooks->get_setting( entry->cookie, setting,
												setting_kind, value_ptr );
	if( status != B_OK && gDebug )
		printf( "%s: get_setting hook failed: %ld, setting=%ld\n",
			dbgname, status, setting );
		
	return status;
}


status_t scan_put_one_setting( const scan_id id, const scan_setting_id setting,
								scan_value *value_ptr, scan_settings_mask *mask )
{
	if( gDebug )
		printf( "%s: entering scan_put_one_setting (%ld)\n", dbgname, setting );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen || entry->state >= kScanStateImageOpen ) {
		if( gDebug )
			printf( "%s: id not open, or image already open\n", dbgname );
		return SCAN_BAD_PHASE;
	}

	status_t status = entry->hooks->put_setting( entry->cookie, setting,
												value_ptr, mask );
	if( status != B_OK && gDebug )
		printf( "%s: put_setting hook failed: %ld, setting=%ld\n",
			dbgname, status, setting );
	
	return status;
}


status_t scan_open_image( const scan_id id )
{
	if( gDebug )
		printf( "%s: entering scan_open_image\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state != kScanStateOpen ) {
		if( gDebug )
			printf( "%s: id not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}
	
	status_t status = entry->hooks->open_image( entry->cookie );
	
	if( status == B_OK )
		entry->state = kScanStateImageOpen;
	else if( gDebug )
		printf( "%s: open_image hook failed\n", dbgname );
		
	return status;
}


status_t scan_close_image( const scan_id id )
{
	if( gDebug )
		printf( "%s: entering scan_close_image\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;
	
	if( entry->state < kScanStateImageOpen ) {
		if( gDebug )
			printf( "%s: image not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}
		
	status_t status = entry->hooks->close_image( entry->cookie );
	
	if( status == B_OK )
		entry->state = kScanStateOpen;
	else if( gDebug )
		printf( "%s: close_image hook failed\n", dbgname );
		
	return status;
}


status_t scan_start( const scan_id id )
{
	if( gDebug )
		printf( "%s: entering scan_start\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen || entry->state >= kScanStateImageOpen ) {
		if( gDebug )
			printf( "%s: id not open, or image already open\n", dbgname );
		return SCAN_BAD_PHASE;
	}
		
	status_t status = entry->hooks->start( entry->cookie );
	
	if( status != B_OK && gDebug )
		printf( "%s: start hook failed\n", dbgname );
		
	return status;
}


status_t scan_data( const scan_id id, void *buffer, int32 *count )
{
	if( gDebug )
		printf( "%s: entering scan_data\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 ) {
		if( gDebug )
			printf( "%s: invalid scan_id\n", dbgname );
		return SCAN_BADID;
	}
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateImageOpen ) {
		if( gDebug )
			printf( "%s: image not open\n", dbgname );
		return SCAN_BAD_PHASE;
	}
	
	status_t result = entry->hooks->data( entry->cookie, buffer, count );
	
	if( result == B_OK )
		entry->state = kScanStateData;
	else if (gDebug )
		printf( "%s: data hook failed\n", dbgname );
		
	return result;
}


bool scan_adf_ready( const scan_id id )
{
	if( gDebug )
		printf( "%s: entering scan_adf_ready\n", dbgname );
		
	if( gScannerList.IndexOf( id ) < 0 )
		return false;
		
	scanner_entry *entry = (scanner_entry *) id;

	if( entry->state < kScanStateOpen )
		return SCAN_BAD_PHASE;
		
	return entry->hooks->adf_ready( entry->cookie );
}


void scan_error_message( const scan_id id, status_t err, char *msg )
{
	scanner_entry *entry = static_cast<scanner_entry*>( id );
	entry->hooks->error_message( entry->cookie, err, msg );
		// take care of libscanbe error codes, if they haven't
		// already been taken care of by the add-on
	if( msg && strlen( msg ) == 0 ) {
		switch( err ) {
		case SCAN_INVALID_SETTING:
		case SCAN_BAD_PHASE:
		case SCAN_BADID:
		case SCAN_BAD_PARAM:
			strcpy( msg, "There was a programming error calling libscanbe." );
			break;
		case SCAN_NO_ADDON:
			strcpy( msg, "An add-on for your scanner was not found." );
			break;
		case SCAN_NO_SCANNER:
			strcpy( msg, "Could not communicate with your scanner." );
			break;
		case SCAN_USER_CANCEL:
			strcpy( msg, "The operation was cancelled." );
			break;
		default:
			strcpy( msg, "A scanning error occurred." );
			break;
		}
	}
}


#pragma mark ---- Other Functions ----

/* Callback for walk_addons, finds the one with a give name passed in
	the walk_info structure. On error conditions, B_OK is still returned
	because I want the directory traversal to finish, just in case the
	one they want to open is further down the list. */
static status_t check_addon_name( const CallbackInfo *info, void *data )
{
	image_id ident = load_add_on( info->path );
	if( ident < B_OK ) {
		if( gDebug )
			printf( "%s: load_add_on() failed on '%s'\n", dbgname, info->path );
		return B_OK;
	}
		
	walk_info *wInfo = (walk_info *) data;
	publish_proc func;
	status_t status = get_image_symbol( ident, kPublishNamesFunction,
								B_SYMBOL_TYPE_TEXT, &func );
	if( status != B_OK ) {
		if( gDebug )
			printf( "%s: no %s in '%s'\n", dbgname, kPublishNamesFunction, info->path );
		unload_add_on( ident );
		return B_OK;
	}
	
	if( gDebug )
		printf( "%s: looking at exported names for '%s'\n", dbgname, info->path );
		
	const char **nameArray = func();
	for( int32 i = 0; nameArray[i]; i++ ) {
		if( gDebug ) {
			printf( "%s: [%ld] %s\n", dbgname, i, nameArray[i] );
			printf( "%s: [%ld] %s\n", dbgname, i+1, nameArray[i+1] );
		}
		if( strcmp( wInfo->name, nameArray[i] ) == 0 ) {
			wInfo->id = ident;
			return B_OK;	// leave loaded to use
		}
		if( ! nameArray[++i] )
			break;
	}
	
	unload_add_on( ident );
	return B_OK;
}

static status_t get_settings( scanner_entry *entry, uint32 kind,
								scan_settings *settings )
{
	status_t status = B_OK;
	scan_value value;
	
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_AREA, kind, &value );
	if( status != B_OK )
		return status;
	settings->scan_area = value.rect;
	
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_IMAGETYPE, kind, &value );
	if( status != B_OK )
		return status;
	settings->image_type = value.type;
	
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_PIXELBITS, kind, &value );
	if( status != B_OK )
		return status;
	settings->pixel_bits = value.u_int;
	
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_RESOLUTION, kind, &value );
	if( status != B_OK )
		return status;
	settings->resolution = value.u_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_BRIGHTNESS, kind, &value );
	if( status != B_OK )
		return status;
	settings->brightness = value.s_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_CONTRAST, kind, &value );
	if( status != B_OK )
		return status;
	settings->contrast = value.s_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_SCALING, kind, &value );
	if( status != B_OK )
		return status;
	settings->scaling = value.s_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_WIDTH, kind, &value );
	if( status != B_OK )
		return status;
	settings->pixel_width = value.u_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_HEIGHT, kind, &value );
	if( status != B_OK )
		return status;
	settings->pixel_height = value.u_int;
		
	status = entry->hooks->get_setting( entry->cookie,
					SCAN_SETTING_ROWBYTES, kind, &value );
	if( status != B_OK )
		return status;
	settings->row_bytes = value.u_int;
	
	return B_OK;
}


static status_t put_settings( scanner_entry *entry,
								scan_settings *settings,
								scan_settings_mask *mask )
{
	status_t status = B_OK;
	scan_value value;
	
	value.rect = settings->scan_area;
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_AREA, &value, mask );
	if( status != B_OK )
		return status;
	
	value.type = settings->image_type;
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_IMAGETYPE, &value, mask );
	if( status != B_OK )
		return status;
	
	value.u_int = settings->pixel_bits;
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_PIXELBITS, &value, mask );
	if( status != B_OK )
		return status;
	
	value.s_int = settings->resolution;
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_RESOLUTION, &value, mask );
	if( status != B_OK )
		return status;
	
	value.s_int = settings->brightness;
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_BRIGHTNESS, &value, mask );
	if( status != B_OK )
		return status;
	
	value.s_int = settings->contrast;	
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_CONTRAST, &value, mask );
	if( status != B_OK )
		return status;
	
	value.s_int = settings->scaling;	
	status = entry->hooks->put_setting( entry->cookie,
					SCAN_SETTING_SCALING, &value, mask );
	if( status != B_OK )
		return status;
		
	return B_OK;
}

/*	Grab a name out of a resource in the library file to point
	at where the add-ons are found. */
static status_t get_addon_dir( BDirectory *dir )
{
	status_t status = B_OK;
	BPath path;
	status = find_directory( B_USER_ADDONS_DIRECTORY, &path );
	if( status != B_OK )
		return status;

	int32 cookie = 0;
	image_info info;
	status_t findStat;
	bool found = false;
	do {
		status = get_next_image_info( 0, &cookie, &info );
		if( status == B_OK ) {
			void *sym;
			findStat = get_image_symbol( info.id, kMyIdentifierName,
										B_SYMBOL_TYPE_DATA, &sym );
			if( findStat == B_OK )
				found = true;
		}
	} while( status != B_BAD_VALUE && !found );
	if( ! found )
		return B_BAD_ADDRESS;
		
	BFile libFile;
	status = libFile.SetTo( info.name, B_READ_ONLY );
	if( status != B_OK )
		return status;
		
	BResources rez;
	status = rez.SetTo( &libFile );
	if( status != B_OK )
		return status;

	size_t size;
	void *rezStr = rez.FindResource( kStringType, kSubdirNameID,
										&size );
	if( ! rezStr )
		return B_RESOURCE_NOT_FOUND;
	
	status = path.Append( (char *) rezStr, true );
	if( status != B_OK )
		goto errXit;
	
	status = dir->SetTo( path.Path() );
	if( status != B_OK )
		goto errXit;

	free( rezStr );
	return B_OK;

errXit:
	free( rezStr );
	return status;
}


static status_t get_addon_info( const BEntry &entry, CallbackInfo &info )
{
	info.path = NULL;
	info.flags = 0;
	info.info[0] = 0;
	
		// path
	BPath path;
	status_t status = entry.GetPath( &path );
	if( status != B_OK )
		return status;
	info.path = new char[ strlen( path.Path() ) + 1 ];
	strcpy( info.path, path.Path() );
	
		// flags
	BFile file( &entry, B_READ_ONLY );
	status = file.InitCheck();
	if( status != B_OK )
		return status;
	BResources rez;
	status = rez.SetTo( &file );
	if( status != B_OK )
		return status;
	size_t size;
	void *ptr = rez.FindResource( 'GUIF', (int32) 0, &size );
	if( ptr  ) {
		if( size == 8 ) {
			uint32 *pFlags = static_cast<uint32*>( ptr );
			if( *pFlags == 1 )
				info.flags |= SCAN_FLAG_UI;
			pFlags++;
			if( *pFlags == 1 )
				info.flags |= SCAN_FLAG_DRIVER;
		}
		free( ptr );
	}
	
		// info
	ptr = rez.FindResource( 'info', (int32) 0, &size );
	if( ptr ) {
		strncpy( info.info, (char *) ptr, SCAN_STRING_LENGTH - 1 );
		info.info[SCAN_STRING_LENGTH - 1] = 0;
		free( ptr );
	}
	
	return B_OK;
}


bool libscanbe_get_scanner( char *name )
{
	if( gPrefs.InitCheck() || gSettings.InitCheck() )
		return false;
	uint32 type;
	ssize_t size;
	void *data;
	status_t status = gSettings.GetData( kPrefScannerName, data,
										size, type );
	if( status == B_OK )
		strcpy( name, (char *) data );

	return status == B_OK;
}


bool libscanbe_save_scanner( const char *name )
{
	if( gPrefs.InitCheck() || gSettings.InitCheck() )
		return false;
	status_t status = gSettings.SetData( kPrefScannerName,
		name, strlen( name ) + 1, B_STRING_TYPE );
	gSettings.Save();
	return status == B_OK;
}


#endif // _LIBSCANBE_H
