#include "common.hpp"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

namespace engine
{
	class win32_window_handler
	{
	private:
		HINSTANCE _module;
		LPCWSTR _class_name;
	private:
		win32_window_handler(WNDPROC handler) : _module(GetModuleHandleW(nullptr)), _class_name(nullptr)
		{
			std::array<WCHAR, 0x30> class_name{};
			error::throw_last_code(create_guid_string(class_name));
			const WNDCLASSEXW window_class{
				.cbSize = sizeof(window_class),
				.style = CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
				.lpfnWndProc = handler,
				.hInstance = this->_module,
				.hIcon = _load_default_icon(window_class.hInstance),
				.hCursor = LoadCursor(nullptr, IDC_ARROW),
				.hbrBackground = GetStockBrush(BLACK_BRUSH),
				.lpszClassName = class_name.data(),
				.hIconSm = window_class.hIcon,
			};
			error::throw_last_code(this->_class_name = (LPCWSTR)RegisterClassExW(&window_class));
		}

		~win32_window_handler()
		{
			error::throw_last_code(UnregisterClassW(this->_class_name, this->_module));
		}
	public:
		static int run(WNDPROC handler, LPCWSTR title, DWORD style, const RECT& rect, LPVOID userdata)
		{
			try
			{
				return win32_window_handler{ handler }._run(title, style, rect, userdata);
			}
			catch (const error& object)
			{
				return object.code();
			}
			return 0;
		}

		static LRESULT WINAPI default_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
		{
			switch (message)
			{
				case WM_NCCREATE:
				case WM_DWMCOLORIZATIONCOLORCHANGED:
				{
					DWORD data = 0, size = sizeof(data);
					if (RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_DWORD, nullptr, &data, &size) == ERROR_SUCCESS)
					{
						DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &(data = ~data), sizeof(data));
					}
					break;
				}

				case WM_DESTROY:
				{
					PostQuitMessage(0);
					break;
				}
			}
			return DefWindowProcW(window, message, wparam, lparam);
		}
	private:
		int _run(LPCWSTR title, DWORD style, const RECT& rect, LPVOID userdata)
		{
			error::throw_last_code(_create_window(title, WS_EX_APPWINDOW, style, rect, userdata));
			return _process(nullptr);
		}

		HWND _create_window(LPCWSTR title, DWORD ex_style, DWORD style, RECT rect, LPVOID userdata) noexcept
		{
			if (SetRect(&rect, rect.left, rect.top, rect.right + rect.left, rect.bottom + rect.top) && AdjustWindowRectEx(&rect, style, false, ex_style))
			{
				return CreateWindowExW(ex_style, this->_class_name, title, style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, this->_module, userdata);
			}
			return nullptr;
		}
	private:
		static HICON _load_default_icon(HINSTANCE module) noexcept
		{
			const auto icon = LoadIcon(module, MAKEINTRESOURCE(1));
			return icon ? icon : LoadIcon(nullptr, IDI_APPLICATION);
		}

		static int _process(HWND window) noexcept
		{
			MSG message{};
			while (GetMessageW(&message, nullptr, 0, 0) > 0)
			{
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			return (int)message.wParam;
		}
	};

	int run_window(WNDPROC handler, LPCWSTR title, DWORD style, const RECT& rect)
	{
		return win32_window_handler::run(handler, title, style, rect, nullptr);
	}

	LRESULT WINAPI default_window_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		return win32_window_handler::default_handler(window, message, wparam, lparam);
	}
}
