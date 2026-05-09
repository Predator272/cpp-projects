#include "window.hpp"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace engine
{
	namespace window
	{
		HICON load_default_icon(HINSTANCE module) noexcept
		{
			const auto icon = LoadIcon(module, MAKEINTRESOURCE(1));
			return icon ? icon : LoadIcon(nullptr, IDI_APPLICATION);
		}

		LPCWSTR register_window_class(HINSTANCE module, WNDPROC handler, LPCVOID param) noexcept
		{
			WCHAR class_name_string[0x40];
			if (SUCCEEDED(StringCchPrintfW(class_name_string, ARRAYSIZE(class_name_string), L"%p", param)))
			{
				const auto icon = load_default_icon(module);
				const WNDCLASSEXW window_class_struct{
					.cbSize = sizeof(window_class_struct),
					.style = CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
					.lpfnWndProc = handler,
					.hInstance = module,
					.hIcon = icon,
					.hCursor = LoadCursor(nullptr, IDC_ARROW),
					.hbrBackground = GetStockBrush(BLACK_BRUSH),
					.lpszClassName = class_name_string,
					.hIconSm = icon,
				};
				return reinterpret_cast<LPCWSTR>(RegisterClassExW(&window_class_struct));
			}
			return nullptr;
		}

		HWND create_window(HINSTANCE module, LPCWSTR class_name, LPCWSTR name, DWORD style, DWORD ex_style, const RECT& rect, HWND parent, LPVOID param) noexcept
		{
			RECT temp = rect;
			if (AdjustWindowRectEx(&temp, style, false, ex_style))
			{
				return CreateWindowExW(ex_style, class_name, name, style, temp.left, temp.top, temp.right - temp.left, temp.bottom - temp.top, parent, nullptr, module, param);
			}
			return nullptr;
		}


		window_class::window_class(WNDPROC handler) :
			_module(GetModuleHandleW(nullptr)), _atom(register_window_class(this->_module, handler, this))
		{
			error::throw_error_if(!this->_atom);
		}

		window_class::~window_class() noexcept
		{
			UnregisterClassW(this->_atom, this->_module);
		}


		window::window(const window_class& object, LPCWSTR name, DWORD style, DWORD ex_style, const RECT& rect, HWND parent, LPVOID param) :
			_hwnd(create_window(object._module, object._atom, name, style, ex_style, rect, parent, param))
		{
			error::throw_error_if(!this->_hwnd);
		}

		window::~window() noexcept
		{
			DestroyWindow(this->_hwnd);
		}


		int window_handler::run(LPCWSTR name, DWORD style, DWORD ex_style, const RECT& rect, HWND parent)
		{
			const window_class _window_class(_static_handler);
			const window _window(_window_class, name, style, ex_style, rect, parent, this);
			MSG message{};
			while (GetMessageW(&message, nullptr, 0, 0))
			{
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			return static_cast<int>(message.wParam);
		}

		LRESULT CALLBACK window_handler::_static_handler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
		{
			if (message == WM_NCCREATE)
			{
				SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCTW>(lparam)->lpCreateParams));
				SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_this_handler));
				return _this_handler(hwnd, message, wparam, lparam);
			}
			return DefWindowProcW(hwnd, message, wparam, lparam);
		}

		LRESULT CALLBACK window_handler::_this_handler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
		{
			if (auto _this = reinterpret_cast<window_handler*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
			{
				return _this->handler(hwnd, message, wparam, lparam);
			}
			return DefWindowProcW(hwnd, message, wparam, lparam);
		}
	}
}
