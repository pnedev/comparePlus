#include "OptionDialog.h"
#include "PluginInterface.h"
#include <Commctrl.h>
#include <shlobj.h>
#include <uxtheme.h>

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

UINT OptionDialog::doDialog(struct sUserSettings * Settings)
{
    _Settings = Settings;
    return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_OPTION_DIALOG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
	//goToCenter();
}

BOOL CALLBACK OptionDialog::run_dlgProc(HWND /*hwnd*/, UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
			goToCenter();

			ETDTProc EnableDlgTheme = (ETDTProc)::SendMessage(_nppData._nppHandle, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
			
            if (EnableDlgTheme != NULL)
            {
                EnableDlgTheme(_hSelf, ETDT_ENABLETAB);
            }

            _ColorComboAdded.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_ADDED_COLOR));
            _ColorComboChanged.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_CHANGED_COLOR));
            _ColorComboBlank.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_BLANK_COLOR));
            _ColorComboMoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_MOVED_COLOR));
            _ColorComboRemoved.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REMOVED_COLOR));
            _ColorComboHighlight.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_HIGHLIGHT_COLOR));

            HWND hUpDown = GetDlgItem(_hSelf, IDC_SPIN_CTL);
            SendMessage(hUpDown, UDM_SETRANGE, 0L, MAKELONG(100, 0));

            SetParams();

            break;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK:
					if (GetParams() == FALSE) return FALSE;
					::EndDialog(_hSelf, IDOK);
					return TRUE;
				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
					return TRUE;
                case IDDEFAULT:
                    _Settings->ColorSettings.added     = DEFAULT_ADDED_COLOR;
                    _Settings->ColorSettings.blank     = DEFAULT_BLANK_COLOR;
                    _Settings->ColorSettings.changed   = DEFAULT_CHANGED_COLOR;
                    _Settings->ColorSettings.deleted   = DEFAULT_DELETED_COLOR;
                    _Settings->ColorSettings.moved     = DEFAULT_MOVED_COLOR;
                    _Settings->ColorSettings.highlight = DEFAULT_HIGHLIGHT_COLOR;
                    _Settings->ColorSettings.alpha     = DEFAULT_HIGHLIGHT_ALPHA;
                    SetParams();
                    break;
				case IDC_COMBO_ADDED_COLOR :
					_ColorComboAdded.onSelect();
                    break;
                case IDC_COMBO_CHANGED_COLOR :
                    _ColorComboChanged.onSelect();
                    break;
                case IDC_COMBO_BLANK_COLOR :
                    _ColorComboBlank.onSelect();
                    break;
                case IDC_COMBO_MOVED_COLOR :
                    _ColorComboMoved.onSelect();
                    break;
                case IDC_COMBO_REMOVED_COLOR :
                    _ColorComboRemoved.onSelect();
					break;
                case IDC_COMBO_HIGHLIGHT_COLOR:
                    _ColorComboHighlight.onSelect();
                    break;
				default :
					return FALSE;
			}
			break;
		}
	}
	return FALSE;
}

void OptionDialog::SetParams(void)
{
    // Set current colors configured in option dialog
    _ColorComboAdded.setColor(_Settings->ColorSettings.added);
    _ColorComboMoved.setColor(_Settings->ColorSettings.moved);
    _ColorComboRemoved.setColor(_Settings->ColorSettings.deleted);
	_ColorComboChanged.setColor(_Settings->ColorSettings.changed);
	_ColorComboBlank.setColor(_Settings->ColorSettings.blank);
    _ColorComboHighlight.setColor(_Settings->ColorSettings.highlight);

    // Set transparency
    SetDlgItemInt(_hSelf, IDC_SPIN_BOX, _Settings->ColorSettings.alpha, FALSE);

    // Set symbols cfg
    HWND hSymbols = GetDlgItem(_hSelf, IDC_CHECK_SYMBOLS);
    if (_Settings->OldSymbols == TRUE)
    {
        SendMessage(hSymbols, BM_SETCHECK, BST_CHECKED, (LPARAM)0);
    }
    else
    {
        SendMessage(hSymbols, BM_SETCHECK, BST_UNCHECKED, (LPARAM)0);
    }
}

BOOL OptionDialog::GetParams(void)
{
    // Get color choosed in dialog
	_ColorComboAdded.getColor((LPCOLORREF)&_Settings->ColorSettings.added);
    _ColorComboMoved.getColor((LPCOLORREF)&_Settings->ColorSettings.moved);
	_ColorComboRemoved.getColor((LPCOLORREF)&_Settings->ColorSettings.deleted);
	_ColorComboChanged.getColor((LPCOLORREF)&_Settings->ColorSettings.changed);
	_ColorComboBlank.getColor((LPCOLORREF)&_Settings->ColorSettings.blank);
    _ColorComboHighlight.getColor((LPCOLORREF)&_Settings->ColorSettings.highlight);

    // Get transparency
    _Settings->ColorSettings.alpha = GetDlgItemInt(_hSelf, IDC_SPIN_BOX, NULL, FALSE);

    // Get symbols
    HWND hSymbols = GetDlgItem(_hSelf, IDC_CHECK_SYMBOLS);
    if (SendMessage(hSymbols, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
    {
        _Settings->OldSymbols = TRUE;
    }
    else
    {
        _Settings->OldSymbols = FALSE;
    }

    return TRUE;
}