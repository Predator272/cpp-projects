#pragma once

#include "engine.hpp"
#include <windows.h>
#include <windowsx.h>
#include <array>
#include <span>

namespace engine
{
	class error
	{
	private:
		const int _code = 0;
	private:
		constexpr explicit error(const int code) noexcept;
	public:
		int code() const noexcept;
	public:
		static void throw_code(const int code);
		static void throw_last_code(const bool expression);
	};

	bool create_guid_string(const std::span<WCHAR>& string) noexcept;
	void show_error(LPCWSTR title, LPCWSTR format, ...) noexcept;

	int run_window(WNDPROC handler, LPCWSTR title, DWORD style, const RECT& rect);
	LRESULT WINAPI default_window_handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
}
