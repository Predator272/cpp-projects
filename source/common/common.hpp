#pragma once

#include "engine.hpp"
#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <tchar.h>

#include <string>
#include <format>
#include <system_error>

namespace engine
{
	constexpr int window_min_size_x = 426;
	constexpr int window_min_size_y = 240;
	constexpr DWORD window_def_style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	constexpr DWORD window_def_ex_style = WS_EX_APPWINDOW;

	std::wstring unicode(const std::string& string) noexcept;
}
