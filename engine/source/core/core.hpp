#pragma once

#include <windows.h>

namespace Engine
{
	namespace Core
	{
		double QueryPerformanceTimer() noexcept;
		bool CreateGUIDString(LPWSTR String, DWORD Size) noexcept;

		class CCrashHandler
		{
		private:
			PVOID _Handler;
		public:
			CCrashHandler() noexcept;
			~CCrashHandler() noexcept;
		};

		class IWindow
		{
		public:
			int Run(LPCWSTR Title, DWORD Style, const RECT& Rect) noexcept;
		protected:
			virtual bool Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept = 0;
		};
	}
}
