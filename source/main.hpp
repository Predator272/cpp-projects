#pragma once

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32")
#include <pathcch.h>
#pragma comment(lib, "pathcch")
#include <shlobj.h>
#pragma comment(lib, "shell32")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi")
#include <shellscalingapi.h>
#pragma comment(lib,"shcore")

#include "resources/resources.hpp"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


template<DWORD Size>
class CResourceString
{
private:
	TCHAR _String[Size]{};
public:
	CResourceString& Load(HINSTANCE Module, UINT StringID) noexcept
	{
		if (!LoadString(Module, StringID, this->_String, ARRAYSIZE(this->_String)))
		{
			ZeroMemory(this->_String, sizeof(this->_String));
		}
		return (*this);
	}

	LPCTSTR Get() const noexcept
	{
		return this->_String;
	}
};

class CComboBox
{
private:
	HWND _Handle;
public:
	CComboBox(HWND Handle = nullptr) noexcept : _Handle{ Handle }
	{
	}

	CComboBox(HWND Window, int ItemID) noexcept : CComboBox(GetDlgItem(Window, ItemID))
	{
	}
public:
	const CComboBox& AddItem(LPCTSTR String, HICON Icon = nullptr) const noexcept
	{
		const auto IconID = this->_AddIcon(Icon);
		COMBOBOXEXITEM ComboBoxItem{
			.mask = (UINT)((IconID != -1) ? CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE : CBEIF_TEXT),
			.iItem = -1,
			.pszText = (LPTSTR)String,
			.iImage = ((IconID != -1) ? IconID : 0),
			.iSelectedImage = ComboBoxItem.iImage,
		};
		this->_SendMessage(CBEM_INSERTITEM, 0, (LPARAM)&ComboBoxItem);
		return (*this);
	}

	const CComboBox& SetCurSel(int Index) const noexcept
	{
		SendMessage(this->_Handle, CB_SETCURSEL, 0, 0);
		return (*this);
	}

	int GetCurSel() const noexcept
	{
		return (int)SendMessage(this->_Handle, CB_GETCURSEL, 0, 0);
	}

	bool GetText(LPTSTR String, DWORD Size) const noexcept
	{
		if (GetWindowText(this->_Handle, String, (int)Size))
		{
			return true;
		}
		ZeroMemory(String, Size);
		return false;
	}

	const CComboBox& Enable(bool Value) const noexcept
	{
		EnableWindow(this->_Handle, Value);
		return (*this);
	}
private:
	LRESULT _SendMessage(UINT Message, WPARAM Param1, LPARAM Param2) const noexcept
	{
		return SendMessage(this->_Handle, Message, Param1, Param2);
	}

	int _AddIcon(HICON Icon) const noexcept
	{
		if (Icon)
		{
			if (const auto ImageList = (HIMAGELIST)this->_SendMessage(CBEM_GETIMAGELIST, 0, 0))
			{
				return ImageList_AddIcon(ImageList, Icon);
			}
			this->_SendMessage(CBEM_SETIMAGELIST, 0, (LPARAM)ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, 0, 0));
			return this->_AddIcon(Icon);
		}
		return -1;
	}
};

class IDialogBox
{
public:
	int Run(HINSTANCE Module, HWND ParentWindow, UINT ResourceID) noexcept
	{
		return (int)DialogBoxParam(Module, MAKEINTRESOURCE(ResourceID), ParentWindow, _InternalHandler, (LPARAM)this);
	}
public:
	virtual bool Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept = 0;
private:
	static INT_PTR WINAPI _InternalHandler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2)
	{
		if (Message == WM_INITDIALOG)
		{
			SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR)Param2);
			const auto Icon = _LoadDefaultIcon((HINSTANCE)GetWindowLongPtr(Window, GWLP_HINSTANCE));
			SendMessage(Window, WM_SETICON, ICON_SMALL, (LPARAM)Icon);
			SendMessage(Window, WM_SETICON, ICON_BIG, (LPARAM)Icon);
		}

		if (auto Pointer = (IDialogBox*)GetWindowLongPtr(Window, GWLP_USERDATA))
		{
			return Pointer->Handler(Window, Message, Param1, Param2);
		}
		return false;
	}

	static HICON _LoadDefaultIcon(HINSTANCE Module) noexcept
	{
		const auto Icon = LoadIcon(Module, MAKEINTRESOURCE(1));
		return Icon ? Icon : LoadIcon(nullptr, IDI_APPLICATION);
	}
};
