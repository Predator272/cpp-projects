#include "engine.hpp"

#include <crtdbg.h>
#include <windowsx.h>
#include <hidsdi.h>
#include <strsafe.h>

#include "core/core.hpp"

namespace Engine
{
	namespace Internal
	{
		class CWindow : public Core::IWindow
		{
		private:
			Engine::IApplication& _Pointer;
		public:
			CWindow(Engine::IApplication* Pointer) noexcept : _Pointer{ *Pointer }
			{
			}
		private:
			bool Handler(HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept override
			{
				return this->_Pointer.Handler(Handle, Type, Argument1, Argument2);
			}
		};
	}

	int IApplication::Run(LPCWSTR Title, DWORD Style, const RECT& Rect) noexcept
	{
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		Core::CCrashHandler CrashHandler;
		return Internal::CWindow{ this }.Run(Title, Style, Rect);
	}
}
