#include "LSWLayoutManager.h"
#include "../Base/LSWBase.h"
#include "../Base/LSWGlobalAlloc.h"
#include "../Button/LSWButton.h"
#include "../CheckButton/LSWCheckButton.h"
#include "../GroupBox/LSWGroupBox.h"
#include "../ListView/LSWListView.h"
#include "../MainWindow/LSWMainWindow.h"
#include "../RadioButton/LSWRadioButton.h"

#include <map>

namespace lsw {

	// == Functions.
	// Creates a window with all of its controls.  Returns the main window widget.
	CWidget * CLayoutManager::CreateWindowX( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal ) {
		if ( !_sTotal ) { return nullptr; }

		CWidget * pwMain = CreateWidget( _pwlLayouts[0], NULL, true );
		if ( !pwMain ) { return nullptr; }

		std::map<DWORD, CWidget *> mIdToPointer;
		mIdToPointer.insert_or_assign( pwMain->Id(), pwMain );
		for ( SIZE_T I = 1; I < _sTotal; ++I ) {
			auto aTemp = mIdToPointer.find( _pwlLayouts[I].dwParentId );
			CWidget * pwParent = aTemp == mIdToPointer.end() ? nullptr : aTemp->second;

			CWidget * pwThis = CreateWidget( _pwlLayouts[I], pwParent, true );
			if ( !pwThis ) {
				// Erase everything from the map and return nullptr.
				for ( auto aIt = mIdToPointer.begin(); aIt != mIdToPointer.end(); aIt++ ) {
					delete aIt->second;
				}
				return nullptr;
			}

			mIdToPointer.insert_or_assign( pwThis->Id(), pwThis );
		}

		return pwMain;
	}

	// Creates a modal dialog with all of its controls.  Returns the dialog exit value.
	INT_PTR CLayoutManager::CreateDialogX( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal, CWidget * _pwParent ) {
		if ( !_sTotal ) { return 0; }
		std::vector<CWidget *> vWidgets;	// On failure, all widgets must be destroyed.
#define LSW_DESTROY_WIDGETS		for ( size_t J = 0; J < vWidgets.size(); ++J ) { delete vWidgets[J]; }


		CWidget * pwMain = CreateWidget( _pwlLayouts[0], _pwParent, false );
		if ( !pwMain ) { return 0; }
		std::map<DWORD, CWidget *> mIdToPointer;
		mIdToPointer.insert_or_assign( pwMain->Id(), pwMain );
		vWidgets.push_back( pwMain );
		for ( SIZE_T I = 1; I < _sTotal; ++I ) {
			auto aTemp = mIdToPointer.find( _pwlLayouts[I].dwParentId );
			CWidget * pwParent = aTemp == mIdToPointer.end() ? nullptr : aTemp->second;
			// Parent here cannot be NULL.
			if ( !pwParent ) { pwParent = pwMain; }

			CWidget * pwThis = CreateWidget( _pwlLayouts[I], pwParent, false );
			if ( !pwThis ) {
				// Erase everything from the map and return 0.
				for ( auto aIt = mIdToPointer.begin(); aIt != mIdToPointer.end(); aIt++ ) {
					delete aIt->second;
				}
				LSW_DESTROY_WIDGETS;
				return 0;
			}

			mIdToPointer.insert_or_assign( pwThis->Id(), pwThis );
			vWidgets.push_back( pwThis );
		}

		// The controls are linked to each other, just not attached to any windows.
		// Figure out how much memory to allocate.
		SIZE_T sSize = LayoutToDialogTemplate( _pwlLayouts, _sTotal, nullptr, vWidgets );
		// Allocate.
		CGlobalAlloc gaAlloc( GMEM_ZEROINIT, sSize );
		DLGTEMPLATE * pdtTemplate = reinterpret_cast<DLGTEMPLATE *>(gaAlloc.Lock());
		LayoutToDialogTemplate( _pwlLayouts, _sTotal, pdtTemplate, vWidgets );
		gaAlloc.Unlock();

		INT_PTR ipRet = ::DialogBoxIndirectParamW( CBase::GetThisHandle(), 
			pdtTemplate, 
			_pwParent->Wnd(), 
			CWidget::DialogProc,
			reinterpret_cast<LPARAM>(&vWidgets) );
		
		if ( ipRet == -1 ) {
			CBase::PrintError( L"CreateDialogX" );
		}
		LSW_DESTROY_WIDGETS;
		/*if ( ipRet == 0 ) {
			return pwMain;
		}*/

		return ipRet;
#undef LSW_DESTROY_WIDGETS
	}

	// Creates a class based on its type.
	CWidget * CLayoutManager::CreateWidget( const LSW_WIDGET_LAYOUT &_wlLayout, CWidget * _pwParent, bool _bCreateWidget ) {
		switch ( _wlLayout.ltType ) {
			case LSW_LT_WIDGET : { return new CWidget( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_BUTTON : { return new CButton( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_LISTVIEW : { return new CListView( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_GROUPBOX : { return new CGroupBox( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_RADIO : { return new CRadioButton( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_CHECK : { return new CCheckButton( _wlLayout, _pwParent, _bCreateWidget ); }
			case LSW_LT_MAINWINDOW : { return new CMainWindow( _wlLayout, _pwParent, _bCreateWidget ); }
		}
		return nullptr;
	}

	// Creates a menu given a menu layout.
	HMENU CLayoutManager::CreateMenu( const LSW_MENU_LAYOUT &_mlLayout ) {
		HMENU hMenu = ::CreateMenu();
		for ( SIZE_T I = 0; I < _mlLayout.stTotalMenuItems; ++I ) {
			if ( !AppendMenuItem( hMenu, _mlLayout.pmiItems[I] ) ) {
				::DestroyMenu( hMenu );
				return NULL;
			}
		}
		return hMenu;
	}

	// Creates menus given menu layouts.
	HMENU CLayoutManager::CreateMenu( const LSW_MENU_LAYOUT * _pmlLayouts, SIZE_T _sTotal ) {
		std::map<DWORD, HMENU> mIdToMenu;
		HMENU hMenu = NULL;
		for ( SIZE_T I = 0; I < _sTotal; ++I ) {
			HMENU hTemp = CreateMenu( _pmlLayouts[I] );
			if ( !hTemp ) {
				for ( auto I = mIdToMenu.begin(); I != mIdToMenu.end(); ++I ) {
					::DestroyMenu( I->second );
				}
				return NULL;
			}
			if ( !hMenu ) {
				hMenu = hTemp;
			}

			// Parent it.
			HMENU hParent = NULL;
			if ( _pmlLayouts[I].dwParentMenuId ) {
				auto aTemp = mIdToMenu.find( _pmlLayouts[I].dwParentMenuId );
				hParent = aTemp == mIdToMenu.end() ? NULL : aTemp->second;
			}
			if ( hParent ) {
				// Add it as a child to the item.
				MENUITEMINFOW miSub = { sizeof( MENUITEMINFOW ) };
				miSub.fMask = MIIM_SUBMENU;
				miSub.hSubMenu = hTemp;
				::SetMenuItemInfoW( hParent, _pmlLayouts[I].dwParentMenuItemId, FALSE, &miSub );
			}

			mIdToMenu.insert_or_assign( _pmlLayouts[I].dwId, hTemp );
		}

		return hMenu;
	}

	// Converts an array of LSW_WIDGET_LAYOUT structures to a DLGTEMPLATE structure and an array of following DLGITEMTEMPLATE structures.
	SIZE_T CLayoutManager::LayoutToDialogTemplate( const LSW_WIDGET_LAYOUT * _pwlLayouts, SIZE_T _sTotal, DLGTEMPLATE * _pdtTemplate, const std::vector<CWidget *> &_vWidgets ) {
		uint8_t * pui8Temp = reinterpret_cast<uint8_t *>(_pdtTemplate);
		SIZE_T sRet = sizeof( DLGTEMPLATE );
		// For now, always set the font.
		DWORD dwStyle = _pwlLayouts[0].dwStyle | DS_SETFONT;
		if ( _pdtTemplate ) {
			_pdtTemplate->style = dwStyle;
			_pdtTemplate->dwExtendedStyle = _pwlLayouts[0].dwStyleEx;
			_pdtTemplate->cdit = static_cast<WORD>(_sTotal - 1);
			_pdtTemplate->x = static_cast<short>(_pwlLayouts[0].iLeft);
			_pdtTemplate->y = static_cast<short>(_pwlLayouts[0].iTop);
			_pdtTemplate->cx = static_cast<short>(_pwlLayouts[0].dwWidth);
			_pdtTemplate->cy = static_cast<short>(_pwlLayouts[0].dwHeight);

		}
		WORD * pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
#define LSW_WRITE_WORD( VAL )	if ( _pdtTemplate ) { (*pwTemp++) = static_cast<WORD>(VAL); } sRet += sizeof( WORD )
		// No menu.
		LSW_WRITE_WORD( 0 );
		
		// Dialog class.  If input is, null, use default.
		if ( !_pwlLayouts[0].lpwcClass ) {
			LSW_WRITE_WORD( 0 );
		}
		else {
			pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
			sRet += ItemTemplateString( _pwlLayouts[0].lpwcClass, _pdtTemplate ? pwTemp : nullptr );
		}

		// Dialog title.
		pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
		sRet += ItemTemplateString( _pwlLayouts[0].pwcText, _pdtTemplate ? pwTemp : nullptr );

		// Optional font.
		if ( dwStyle & DS_SETFONT ) {
			// Get the system default.
			NONCLIENTMETRICSW ncmMetrics;
			ncmMetrics.cbSize = sizeof( ncmMetrics );
			::SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, ncmMetrics.cbSize, &ncmMetrics, 0 );
			//HFONT hFont = ::CreateFontIndirectW( &ncmMetrics.lfMessageFont );

			pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
			LSW_WRITE_WORD( ncmMetrics.lfMessageFont.lfHeight );
			pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
			sRet += ItemTemplateString( ncmMetrics.lfMessageFont.lfFaceName, _pdtTemplate ? pwTemp : nullptr );
		}

		sRet = reinterpret_cast<SIZE_T>(CHelpers::DwordAlign( reinterpret_cast<LPWORD>(sRet) ));

		// All other controls.
		for ( SIZE_T I = 1; I < _sTotal; ++I ) {
			sRet = reinterpret_cast<SIZE_T>(CHelpers::DwordAlign( reinterpret_cast<LPWORD>(sRet) ));
			DLGITEMTEMPLATE * pditTemp = reinterpret_cast<DLGITEMTEMPLATE *>(pui8Temp + sRet);
			sRet += LayoutToItemTemplate( _pwlLayouts[I], _pdtTemplate ? pditTemp : nullptr, _vWidgets[I] );
		}

		return reinterpret_cast<SIZE_T>(CHelpers::DwordAlign( reinterpret_cast<LPWORD>(sRet) ));
#undef LSW_WRITE_WORD
	}

	// Converts an LSW_WIDGET_LAYOUT structure to a DLGITEMTEMPLATE structure.  Pass nullptr to get the size to allocate.
	SIZE_T CLayoutManager::LayoutToItemTemplate( const LSW_WIDGET_LAYOUT &_wlLayout, DLGITEMTEMPLATE * _pdittTemplate, CWidget * _pwCreationData ) {
		uint8_t * pui8Temp = reinterpret_cast<uint8_t *>(_pdittTemplate);
		SIZE_T sRet = sizeof( DLGITEMTEMPLATE );
		if ( _pdittTemplate ) {
			_pdittTemplate->style = _wlLayout.dwStyle;
			_pdittTemplate->dwExtendedStyle = _wlLayout.dwStyleEx;
			_pdittTemplate->id = _wlLayout.wId;
			_pdittTemplate->x = static_cast<short>(_wlLayout.iLeft);
			_pdittTemplate->y = static_cast<short>(_wlLayout.iTop);
			_pdittTemplate->cx = static_cast<short>(_wlLayout.dwWidth);
			_pdittTemplate->cy = static_cast<short>(_wlLayout.dwHeight);
		}
		WORD * pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
		sRet += ItemTemplateString( _wlLayout.lpwcClass, _pdittTemplate ? pwTemp : nullptr );

		pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
		sRet += ItemTemplateString( _wlLayout.pwcText, _pdittTemplate ? pwTemp : nullptr );

		// No creation data.
		if ( _pdittTemplate ) {
			pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
			(*pwTemp++) = 0;
		}
		sRet += sizeof( WORD );
		/*
		if ( _pdittTemplate ) {
			pwTemp = reinterpret_cast<WORD *>(pui8Temp + sRet);
			(*pwTemp++) = sizeof( _pwCreationData ) + sizeof( WORD );	// Size of the pointer is correct.
			std::memcpy( pwTemp, _pwCreationData, sizeof( _pwCreationData ) );
		}
		sRet += sizeof( _pwCreationData ) + sizeof( WORD );*/
		return sRet;//reinterpret_cast<SIZE_T>(CHelpers::DwordAlign( reinterpret_cast<LPWORD>(sRet) ));
	}

	// Writes a string to a given pointer formatted for use with DLGITEMTEMPLATE objects.
	SIZE_T CLayoutManager::ItemTemplateString( const WCHAR * _pwcString, WORD * _pwDest ) {
		SIZE_T sRet = 0;
		if ( _pwcString ) {
			while ( (*_pwcString) ) {
				if ( _pwDest ) {
					(*_pwDest++) = (*_pwcString);
				}

				sRet += sizeof( WORD );
				++_pwcString;
			}
		}
		// The NULL terminator.
		if ( _pwDest ) {
			(*_pwDest) = L'\0';
		}
		sRet += sizeof( WORD );
		return sRet;
	}

	// Appends a menu item to a menu.
	BOOL CLayoutManager::AppendMenuItem( HMENU _hMenu, const LSW_MENU_ITEM &_miItem ) {
		if ( _miItem.bIsSeperator ) {
			return ::AppendMenuW( _hMenu, MF_SEPARATOR, 0, NULL );
		}
		UINT uiFlags = MF_STRING;
		if ( _miItem.bCheckable ) {
			uiFlags |= _miItem.bChecked ? MF_CHECKED : MF_UNCHECKED;
		}
		uiFlags |= _miItem.bEnabled ? MF_ENABLED : MF_GRAYED;
		return ::AppendMenuW( _hMenu, uiFlags, _miItem.dwId, _miItem.lpwcText );
	}

}	// namespace lsw
