#pragma once

#include <windows.h>
#include <windowsx.h>

namespace Engine
{
	class IHandler
	{
	public:
		virtual bool Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept = 0;
	};

	class IApplication : public IHandler
	{
	public:
		int Run(LPCWSTR Title, DWORD Style, const RECT& Rect) noexcept;
	};
}
