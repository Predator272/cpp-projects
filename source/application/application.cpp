#include "common/common.hpp"
#include "window.hpp"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

namespace engine
{
	class window_impl : public application::window
	{
	public:
		explicit window_impl(HWND hwnd) noexcept : _hwnd(hwnd)
		{
		}

		HWND hwnd() const noexcept override
		{
			return this->_hwnd;
		}

		SIZE size() const noexcept override
		{
			const auto rect = this->rect();
			return { rect.right - rect.left, rect.bottom - rect.top };
		}

		void title(const std::string& string) const noexcept override
		{
			SetWindowTextW(this->_hwnd, unicode(string).c_str());
		}

		void close() const noexcept override
		{
			DestroyWindow(this->_hwnd);
		}
	private:
		RECT rect() const noexcept
		{
			RECT rect;
			if (GetClientRect(this->_hwnd, &rect))
			{
				return rect;
			}
			return {};
		}
	private:
		HWND _hwnd;
	};

	class event_impl : public application::event
	{
	public:
		explicit event_impl(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept : _window(hwnd), _data{ message, wparam, lparam }
		{
		}

		const application::window& window() const noexcept
		{
			return this->_window;
		}

		UINT type() const noexcept
		{
			return this->_data.message;
		}

		WPARAM wparam() const noexcept
		{
			return this->_data.wparam;
		}

		LPARAM lparam() const noexcept
		{
			return this->_data.lparam;
		}
	private:
		const window_impl _window;
		const struct
		{
			UINT message;
			WPARAM wparam;
			LPARAM lparam;
		} _data;
	};

	class main_window_handler : public engine::window::window_handler
	{
	public:
		explicit main_window_handler(application* object) : _application(object)
		{
		}

		int run(const std::string& name, DWORD style, const RECT& rect)
		{
			const auto result = engine::window::window_handler::run(unicode(name).c_str(), style, window_def_ex_style, rect, nullptr);
			if (this->_exception_ptr)
			{
				std::rethrow_exception(this->_exception_ptr);
			}
			return result;
		}
	private:
		LRESULT handler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept override
		{
			try
			{
				if (this->_application)
				{
					const event_impl event(hwnd, message, wparam, lparam);
					if (const auto result = this->_application->handler(event))
					{
						return *result;
					}
				}
			}
			catch (...)
			{
				this->_exception_ptr = std::current_exception();
				this->_application = nullptr;
				PostQuitMessage(0);
			}
			return default_handler(hwnd, message, wparam, lparam);
		}

		static LRESULT default_handler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept
		{
			switch (message)
			{
				case WM_NCCREATE:
				{
					SendMessageW(hwnd, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"ImmersiveColorSet"));
					break;
				}

				case WM_GETMINMAXINFO:
				{
					RECT rect{ 0, 0, window_min_size_x, window_min_size_y };
					if (AdjustWindowRectEx(&rect, GetWindowStyle(hwnd), IsMenu(GetMenu(hwnd)), GetWindowExStyle(hwnd)))
					{
						auto info = reinterpret_cast<LPMINMAXINFO>(lparam);
						info->ptMinTrackSize.x = rect.right - rect.left;
						info->ptMinTrackSize.y = rect.bottom - rect.top;
					}
					break;
				}

				case WM_SETTINGCHANGE:
				{
					LPCWSTR string = reinterpret_cast<LPCWSTR>(lparam);
					if (string && wcscmp(string, L"ImmersiveColorSet") == 0)
					{
						DWORD data = 0, size = sizeof(data);
						if (RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_DWORD, nullptr, &data, &size) == ERROR_SUCCESS)
						{
							const BOOL value = (data == 0);
							DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
						}
						return 0;
					}
					break;
				}

				case WM_DESTROY:
				{
					PostQuitMessage(0);
					break;
				}
			}
			return DefWindowProcW(hwnd, message, wparam, lparam);
		}
	private:
		application* _application;
		std::exception_ptr _exception_ptr;
	};

	int application::run(const config_t& config)
	{
		RECT rect;
		rect.right = std::max<std::int32_t>(config.size.x, window_min_size_x);
		rect.bottom = std::max<std::int32_t>(config.size.y, window_min_size_y);
		rect.left = config.position.x ? config.position.x : (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
		rect.top = config.position.y ? config.position.y : (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;
		rect.right += rect.left;
		rect.bottom += rect.top;

		return main_window_handler(this).run(config.title, config.style ? config.style : window_def_style, rect);
	}
}
