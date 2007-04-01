/* ScannerBe preferences applet. Copyright © Jim Moy, 1997, All rights reserved. */

#include <Application.h>
#include <Bitmap.h>
#include <Window.h>
#include <stdio.h>
#include "ScannerBe.h"
#include "ScanBeConst.h"
#include "Preferences.h"

const char *kSig = "application/x-vnd.jbm-scanpref";
const char *kNoScannerName = "libscanbe_no_scanner";
const uint32 kInvokeMsg = 'linv';

// libscanbe entry points which are not part of the ScannerBe API
extern "C" bool libscanbe_save_scanner( const char *name );
extern "C" bool libscanbe_get_scanner( char *name );

											#pragma mark ---- PrefView Class ----

class PrefView : public BView {
public:
					PrefView( BRect rect );
static	int32		get_addon_info( const CallbackInfo *info, void *data );
virtual	void		AttachedToWindow();
virtual	void		MessageReceived( BMessage *msg );

		char		_current[SCAN_STRING_LENGTH];
		int32		_init_select;
		BListView	*_list;
};

											#pragma mark ---- PrefWindow Class ----

class PrefWindow : public BWindow {
public:
					PrefWindow();
virtual	bool		QuitRequested();
		
		PrefView*	_view;
};

											#pragma mark ---- PrefApp Class ----

class PrefApp : public BApplication {
public:
					PrefApp() : BApplication( kSig )
						{ _window = new PrefWindow; }

		PrefWindow*	_window;
};

											#pragma mark ---- ScannerItem Class ----

// This class keeps track of the friendly scanner info as 
// well as theScannerBe add-on name.
class ScannerItem : public BStringItem {
public:
	ScannerItem( const char *text, const char *info, const char *moreInfo );
	char *_info;
	char *_more_info;
};

											#pragma mark ---- Functions ----

void main() { PrefApp app; app.Run(); }

PrefWindow::PrefWindow()
	: BWindow( BRect( 100, 100, 390, 280 ), "Scanner",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BRect viewRect( Bounds() );
	_view = new PrefView( viewRect );
	AddChild( _view );
	Show();
}

bool PrefWindow::QuitRequested()
{
	int32 sel = _view->_list->CurrentSelection();
	char *scanner = NULL;
	if( sel >= 0 ) {
		BListItem *listItem = _view->_list->ItemAt( sel );
		ScannerItem *item = dynamic_cast<ScannerItem*>( listItem );
		scanner = item->_info;
	} else {
		scanner = (char *) kNoScannerName;
	}
	
	if( ! libscanbe_save_scanner( scanner ) ) {
		BAlert *alert = new BAlert( "",
			"Sorry, couldn't save the scanner preference.",
			"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
		alert->Go();
	}
	
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}

PrefView::PrefView( BRect frame ) : BView( frame, "", B_FOLLOW_ALL, B_WILL_DRAW )
{
	rgb_color back = { 0xEE, 0xEE, 0xEE, 0 };
	SetViewColor( back );
	
	BRect bounds( Bounds() );
	BRect r( bounds );
	
	r.OffsetBy( 10, 10 );
	font_height fHeight;
	GetFontHeight( &fHeight );
	int32 lineHeight = fHeight.ascent + fHeight.descent + fHeight.leading;
	r.bottom = r.top + lineHeight;
	BStringView *string = new BStringView( r, "",
		"Select a scanner:      (double-click for info)" );
	AddChild( string );
										// scanner list
	r = bounds;
	r.InsetBy( 20, 20 + 3 * lineHeight );
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.top = string->Frame().bottom + 8;
	_list = new BListView( r, "" );
	BScrollView *scroll = new BScrollView( "", _list, B_FOLLOW_ALL,
											0, FALSE, TRUE );
	_list->MakeFocus();
	AddChild( scroll );
										// public service announcement
	r = bounds;
	r.InsetBy( 10, 10 );
	r.top = scroll->Frame().bottom + 8;
	r.bottom = r.top + lineHeight;
	string = new BStringView( r, "",
			"The selected scanner will be used by applications " );
	AddChild( string );
	r.OffsetBy( 0, lineHeight );
	string = new BStringView( r, "",
			"supporting the ScannerBe scanning interface." );
	AddChild( string );
	r.OffsetBy( 0, lineHeight );
	string = new BStringView( r, "",
			"See http://www.verinet.com/~jbm/be/scanner" );
	AddChild( string );
										// set up the scanner list
	libscanbe_get_scanner( _current );
	_init_select = -1;
	status_t status = scan_get_addons( get_addon_info, this );
}

void PrefView::AttachedToWindow()
{
	// Hmm, how come I can't do this in the constructor?
	if( _init_select != -1 )
		_list->Select( _init_select );
		
	_list->SetInvocationMessage( new BMessage( kInvokeMsg ) );
	_list->SetTarget( this );
}

void PrefView::MessageReceived( BMessage *msg )
{
	BAlert *alert = NULL;
	switch( msg->what ) {
	case kInvokeMsg:
		int32 sel = _list->CurrentSelection();
		if( sel >= 0 ) {
			BListItem *listItem = _list->ItemAt( sel );
			ScannerItem *item = dynamic_cast<ScannerItem*>( listItem );
			char *str = item->_more_info;
			if( strlen( item->_more_info ) == 0 )
				str = "(No extra file information found.)";
			alert = new BAlert( "", str, "OK" );
			alert->Go();
		}
		break;
	default:
		BView::MessageReceived( msg );
		break;
	}
}

/* Callback from libscanbe_walk_addons. All the return codes are 0 because
	even in the event of a failed operation, I want to keep looking at other
	add-ons. */

int32 PrefView::get_addon_info( const CallbackInfo *info, void *data )
{
	image_id id = load_add_on( info->path );
	if( id < B_OK )
		return 0;
		
	PrefView *view = static_cast<PrefView*>( data );
	publish_proc func;
	status_t status = get_image_symbol( id, kPublishNamesFunction,
								B_SYMBOL_TYPE_TEXT, &func );
	if( status != B_OK )
		return 0;
	
		// only the ones supporting a UI
	if( ( info->flags & SCAN_FLAG_UI ) == 0 )
		return 0;
		
		// all of the names therein
	BListView *list = view->_list;
	const char **nameArray = func();
	for( int32 i = 0; nameArray[i]; i++ ) {
		const char *addonName = nameArray[i];
		const char *addonInfo = nameArray[++i];
		ScannerItem *item = new ScannerItem( addonInfo, addonName, info->info );
		list->AddItem( item );
			// setup current selection
		if( strcmp( addonName, view->_current ) == 0 )
			view->_init_select = list->CountItems() - 1;
	}
	
	return 0;
}

ScannerItem::ScannerItem( const char *text, const char *info, const char *moreInfo )
	: BStringItem( text )
{
	_info = new char[SCAN_STRING_LENGTH];
	strcpy( _info, info );
	_more_info = new char[SCAN_STRING_LENGTH];
	strcpy( _more_info, moreInfo );
}

