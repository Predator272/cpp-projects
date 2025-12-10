#pragma once

#include <windows.h>
#include <windowsx.h>

namespace engine
{
	double clock() noexcept;

	void throw_code(const int code);
	void throw_last_code(const bool expression);

	class event
	{
	public:
		virtual HWND window() const noexcept = 0;
		virtual UINT type() const noexcept = 0;
		virtual WPARAM wparam() const noexcept = 0;
		virtual WPARAM lparam() const noexcept = 0;
	};

	bool main_handler(const event& event);
}
