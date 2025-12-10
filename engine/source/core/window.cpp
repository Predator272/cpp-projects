#include "core.hpp"

#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

EXTERN_C __declspec(dllexport) const int NvOptimusEnablement = 1;
EXTERN_C __declspec(dllexport) const int AmdPowerXpressRequestHighPerformance = 1;

namespace Engine
{
	namespace Core
	{
		class CWindow : public IWindow
		{
		public:
			static int Run(WNDPROC Handler, LPCWSTR Title, DWORD ExStyle, DWORD Style, const RECT& Rect, LPVOID UserData) noexcept
			{
				const auto Module = GetModuleHandleW(nullptr);
				if (const auto ClassName = RegisterWindowClass(Module, Handler))
				{
					if (Create(Module, ClassName, Title, ExStyle, Style, Rect, UserData))
					{
						SetLastError((DWORD)Process(nullptr));
					}
					UnregisterWindowClass(Module, ClassName);
				}
				return (int)HRESULT_FROM_WIN32(GetLastError());
			}

			static LRESULT WINAPI DefaultHandler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept
			{
				switch (Message)
				{
					case WM_NCCREATE:
					case WM_DWMCOLORIZATIONCOLORCHANGED:
					{
						DWORD Data = 0, Size = sizeof(Data);
						if (RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_DWORD, NULL, &Data, &Size) == ERROR_SUCCESS)
						{
							const BOOL Attribute = (Data == 0);
							DwmSetWindowAttribute(Window, DWMWA_USE_IMMERSIVE_DARK_MODE, &Attribute, sizeof(Attribute));
						}
						break;
					}

					case WM_DESTROY:
					{
						PostQuitMessage(0);
						break;
					}
				}
				return DefWindowProcW(Window, Message, Param1, Param2);
			}

			static LRESULT WINAPI StartupHandler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept
			{
				if (Message == WM_NCCREATE)
				{
					SetWindowLongPtrW(Window, GWLP_USERDATA, (LONG_PTR)(((LPCREATESTRUCTW)Param2)->lpCreateParams));
					SetWindowLongPtrW(Window, GWLP_WNDPROC, (LONG_PTR)RedirectHandler);
				}
				return RedirectHandler(Window, Message, Param1, Param2);
			}
		private:
			static LRESULT WINAPI RedirectHandler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept
			{
				const auto Object = (CWindow*)GetWindowLongPtrW(Window, GWLP_USERDATA);
				if (Object && Object->Handler(Window, Message, Param1, Param2))
				{
					return 0;
				}
				return DefaultHandler(Window, Message, Param1, Param2);
			}
		private:
			static HICON LoadDefaultIcon(HINSTANCE Module) noexcept
			{
				const auto Icon = LoadIcon(Module, MAKEINTRESOURCE(1));
				return Icon ? Icon : LoadIcon(nullptr, IDI_APPLICATION);
			}

			static HCURSOR LoadDefaultCursor(HINSTANCE Module) noexcept
			{
				const auto Cursor = LoadCursor(Module, MAKEINTRESOURCE(1));
				return Cursor ? Cursor : LoadCursor(nullptr, IDC_ARROW);
			}

			static LPCWSTR RegisterWindowClass(HINSTANCE Module, WNDPROC Handler) noexcept
			{
				WCHAR ClassName[0x40];
				if (CreateGUIDString(ClassName, ARRAYSIZE(ClassName)))
				{
					const WNDCLASSEXW WindowClass{
						.cbSize = sizeof(WindowClass),
						.style = CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
						.lpfnWndProc = Handler,
						.hInstance = Module,
						.hIcon = LoadDefaultIcon(WindowClass.hInstance),
						.hCursor = LoadDefaultCursor(WindowClass.hInstance),
						.hbrBackground = GetStockBrush(BLACK_BRUSH),
						.lpszClassName = ClassName,
						.hIconSm = WindowClass.hIcon,
					};
					return (LPCWSTR)RegisterClassExW(&WindowClass);
				}
				return nullptr;
			}

			static void UnregisterWindowClass(HINSTANCE Module, LPCWSTR ClassName) noexcept
			{
				const auto LastError = GetLastError();
				if (!UnregisterClassW(ClassName, Module) && LastError)
				{
					SetLastError(LastError);
				}
			}

			static HWND Create(HINSTANCE Module, LPCWSTR ClassName, LPCWSTR Title, DWORD ExStyle, DWORD Style, RECT Rect, LPVOID UserData) noexcept
			{
				if (SetRect(&Rect, Rect.left, Rect.top, Rect.right + Rect.left, Rect.bottom + Rect.top) && AdjustWindowRectEx(&Rect, Style, false, ExStyle))
				{
					return CreateWindowExW(ExStyle, ClassName, Title, Style, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, nullptr, nullptr, Module, UserData);
				}
				return nullptr;
			}

			static int Process(HWND Window) noexcept
			{
				MSG Message{};
				while (GetMessageW(&Message, nullptr, 0, 0) > 0)
				{
					TranslateMessage(&Message);
					DispatchMessageW(&Message);
				}
				return (int)Message.wParam;
			}
		};

		int IWindow::Run(LPCWSTR Title, DWORD Style, const RECT& Rect) noexcept
		{
			return CWindow::Run(CWindow::StartupHandler, Title, WS_EX_APPWINDOW, Style, Rect, this);
		}
	}
}
