#pragma once

#include "../LSWWin.h"
#include "../Widget/LSWWidget.h"
#include "LSWMenuLayout.h"

#include <vector>

namespace lsw {

	class CLayoutManager {
	public :
		// == Functions.
		// Creates a window with all of its controls.  Returns the main window widget.
		CWidget *						CreateWindowX( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal );

		// Creates a dialog with all of its controls.  Returns the dialog widget.
		CWidget *						CreateDialogX( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal, BOOL _bModal, CWidget * _pwParent );

		// Creates a class based on its type.
		virtual CWidget *				CreateWidget( const LSW_WIDGET_LAYOUT &_wlLayout, CWidget * _pwParent, bool _bCreateWidget );

		// Creates a menu given a menu layout.
		HMENU							CreateMenu( const LSW_MENU_LAYOUT &_mlLayout );

		// Creates menus given menu layouts.
		HMENU							CreateMenu( const LSW_MENU_LAYOUT * _pmlLayouts, SIZE_T _sTotal );


	protected :
		// == Functions.
		// Converts an array of LSW_WIDGET_LAYOUT structures to a DLGTEMPLATE structure and an array of following DLGITEMTEMPLATE structures.
		SIZE_T							LayoutToDialogTemplate( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal, DLGTEMPLATE * _pdtTemplate, const std::vector<CWidget *> &_vWidgets );

		// Converts an LSW_WIDGET_LAYOUT structure to a DLGITEMTEMPLATE structure.  Pass nullptr to get the size to allocate.
		SIZE_T							LayoutToItemTemplate( const LSW_WIDGET_LAYOUT &_wlLayout, DLGITEMTEMPLATE * _pdittTemplate, CWidget * _pwCreationData );

		// Writes a string to a given pointer formatted for use with DLGITEMTEMPLATE objects.
		SIZE_T							ItemTemplateString( const WCHAR * _pwcString, WORD * _pwDest );

		// Creates a MENUITEMINFOW structure based off an LSW_MENU_ITEM structure.
		MENUITEMINFOW					ItemToMenuItem( const LSW_MENU_ITEM &_miItem );

		// Appends a menu item to a menu.
		BOOL							AppendMenuItem( HMENU _hMenu, const LSW_MENU_ITEM &_miItem );

	};

}	// namespace lsw