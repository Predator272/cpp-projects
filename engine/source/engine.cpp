#include "common.hpp"

namespace engine
{
	class internal_event : public event
	{
	private:
		HWND _window;
		UINT _message;
		WPARAM _wparam;
		LPARAM _lparam;
	public:
		explicit internal_event(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept :
			_window(window), _message(message), _wparam(wparam), _lparam(lparam)
		{
		}
	public:
		HWND window() const noexcept override
		{
			return this->_window;
		}

		UINT type() const noexcept override
		{
			return this->_message;
		}

		WPARAM wparam() const noexcept override
		{
			return this->_wparam;
		}

		WPARAM lparam() const noexcept override
		{
			return this->_lparam;
		}
	};

	LRESULT WINAPI internal_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		try
		{
			if (main_handler(internal_event(window, message, wparam, lparam)))
			{
				return 0;
			}
		}
		catch (const error& object)
		{
			SetWindowLongPtrW(window, GWLP_WNDPROC, (LONG_PTR)default_window_handler);
			DestroyWindow(window);
			PostQuitMessage(object.code());
			return 0;
		}
		return default_window_handler(window, message, wparam, lparam);
	}

	int entry_point() noexcept
	{
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		const LONG width = 640, height = 480;
		const LONG left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2, top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
		return run_window(internal_handler, GetCommandLineW(), WS_OVERLAPPEDWINDOW, { left, top, width, height });
	}
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return engine::entry_point();
}

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	return engine::entry_point();
}
