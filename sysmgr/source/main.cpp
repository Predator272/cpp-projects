#include "main.hpp"


class CMainDialog : public IDialogBox
{
private:
	bool Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept override
	{
		const auto Module = (HINSTANCE)GetWindowLongPtr(Window, GWLP_HINSTANCE);
		switch (Message)
		{
			case WM_INITDIALOG:
			{
				static constexpr UINT ResourceID1[]{ 0x0001, 0x0002, 0x0003 };
				for (const auto Element : ResourceID1)
				{
					CComboBox(Window, 0x0001).AddItem(CResourceString<64>().Load(Module, Element).Get());
				}
				CComboBox(Window, 0x0001).SetCurSel(0);

				static constexpr UINT ResourceID2[]{ 0x1000, 0x1001, 0x1002, 0x1003, 0x1004, 0x1005, 0x1006 };
				for (const auto Element : ResourceID2)
				{
					CComboBox(Window, 0x0003).AddItem(CResourceString<64>().Load(Module, Element).Get(), LoadIcon(Module, MAKEINTRESOURCE(Element)));
				}
				CComboBox(Window, 0x0003).SetCurSel(0).Enable(false);
				return true;
			}

			case WM_COMMAND:
			{
				switch (LOWORD(Param1))
				{
					case 0x0001:
					{
						if (HIWORD(Param1) == CBN_SELENDOK)
						{
							static constexpr EXECUTION_STATE Flags[]{
								ES_CONTINUOUS,
								ES_CONTINUOUS | ES_SYSTEM_REQUIRED,
								ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED,
							};
							SetThreadExecutionState(Flags[CComboBox(Window, 0x0001).GetCurSel() % ARRAYSIZE(Flags)]);
						}
						return true;
					}

					case 0x0002:
					{
						TCHAR FileName[MAX_PATH]{};
						if (HIWORD(Param1) == CBN_EDITCHANGE && CComboBox(Window, 0x0002).GetText(FileName, ARRAYSIZE(FileName)))
						{
							CComboBox(Window, 0x0003).Enable(PathIsDirectory(FileName));
						}
						return true;
					}

					case 0x0003:
					{
						if (HIWORD(Param1) == CBN_SELENDOK)
						{
							TCHAR ModuleFileName[MAX_PATH]{}, Path[MAX_PATH]{}, DesktopFileName[MAX_PATH]{};
							const auto Index = CComboBox(Window, 0x0003).GetCurSel();
							if ((Index != CB_ERR) &&
								GetModuleFileName(Module, ModuleFileName, ARRAYSIZE(ModuleFileName)) &&
								CComboBox(Window, 0x0002).GetText(Path, ARRAYSIZE(Path)) &&
								SUCCEEDED(StringCchCopy(DesktopFileName, ARRAYSIZE(DesktopFileName), Path)) &&
								PathAppend(DesktopFileName, TEXT("desktop.ini")))
							{
								DeleteFile(DesktopFileName);
								SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_NOTIFYRECURSIVE, DesktopFileName, NULL);
								SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_NOTIFYRECURSIVE, Path, NULL);
								SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_NOTIFYRECURSIVE, Path, NULL);
								if (Index != 0)
								{
									const auto ResourceID = Index + 0x1000;
									SHFOLDERCUSTOMSETTINGS FolderCustomSettings{
										.dwSize = sizeof(FolderCustomSettings),
										.dwMask = FCSM_ICONFILE,
										.pszIconFile = ModuleFileName,
										.iIconIndex = -ResourceID,
									};
									SHGetSetFolderCustomSettings(&FolderCustomSettings, Path, FCS_FORCEWRITE);
									SHSetLocalizedName(Path, ModuleFileName, ResourceID);
								}
							}
						}
						return true;
					}
				}
				break;
			}

			case WM_CLOSE:
			{
				EndDialog(Window, 0);
				return true;
			}
		}
		return false;
	}
};

_Use_decl_annotations_
int WINAPI _tWinMain(HINSTANCE Module, HINSTANCE, LPWSTR CmdLine, int CmdShow)
{
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	return CMainDialog().Run(Module, HWND_DESKTOP, 0x0001) ? EXIT_FAILURE : EXIT_SUCCESS;
}
