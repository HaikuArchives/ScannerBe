/*
	ScannerBe Add On -- Scanner interface for BeOS. 9/23/97
	Copyright (c) 1997, Jim Moy, All Rights Reserved
*/

#ifndef _SCANNERADDON_H
#define _SCANNERADDON_H

#include <SupportDefs.h>
#include "ScannerBe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* these hooks are how libscanbe accesses the add-on */

typedef status_t (*scan_open_hook)( scan_version *version, void **cookie );
typedef status_t (*scan_close_hook)( void *cookie );
typedef status_t (*scan_get_capabilities_hook)( void *cookie, scan_settings_mask *mask );
typedef status_t (*scan_get_setting_hook)( void *cookie, const scan_setting_id id, 
								const scan_setting_kind setting_kind,
								scan_value *value_ptr );
typedef status_t (*scan_put_setting_hook)( void *cookie, const scan_setting_id id, 
								scan_value *value_ptr, scan_settings_mask *mask );
typedef status_t (*scan_open_image_hook)( void *cookie );
typedef status_t (*scan_close_image_hook)( void *cookie );
typedef status_t (*scan_start_hook)( void *cookie );
typedef status_t (*scan_data_hook)( void *cookie, void *buffer, int32 *count );
typedef bool (*scan_adf_ready_hook)( void *cookie );
typedef void (*scan_error_message_hook)( void *cookie, status_t err, char *msg );

/* entry points for the scanner add-on */

typedef struct {
	scan_open_hook				open;
	scan_close_hook				close;
	scan_get_capabilities_hook	get_capabilities;
	scan_get_setting_hook		get_setting;
	scan_put_setting_hook		put_setting;
	scan_open_image_hook		open_image;
	scan_close_image_hook		close_image;
	scan_start_hook				start;
	scan_data_hook				data;
	scan_adf_ready_hook			adf_ready;
	scan_error_message_hook		error_message;
} scan_hooks;


extern const char	**publish_scanners();
extern scan_hooks	*find_scanner( const char *name );

#ifdef __cplusplus
}
#endif

#endif /* _SCANNERADDON_H */
