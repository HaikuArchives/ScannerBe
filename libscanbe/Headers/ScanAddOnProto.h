#pragma once

/* Prototypes, for the convenience of the ScannerBe add-on writer. */

#ifdef __cplusplus
extern "C" {
#endif

static status_t scn_open( scan_version *version, void **cookie );
									
static status_t scn_close( void	 *cookie );

static status_t scn_get_capabilities( void *cookie, scan_settings_mask *caps_mask );

static status_t scn_get_setting(	void				*cookie,
									scan_setting_id		setting_id, 
									scan_setting_kind	setting_kind,
									scan_value			*value_ptr );
								
static status_t scn_put_setting(	void				*cookie,
									scan_setting_id		setting_id, 
									scan_value			*value_ptr,
									scan_settings_mask	*settings_mask );
								
static status_t scn_open_image( void *cookie );

static status_t scn_close_image( void *cookie );

static status_t scn_start( void *cookie );

static status_t scn_data( void *cookie, void *buffer, int32*count );

static bool scn_adf_ready( void *cookie );

static void scn_error_message( void *cookie, status_t err, char *msg );

#ifdef __cplusplus
}
#endif

