#pragma once

#include "common/common.hpp"
#include <windows.h>
#include <system_error>

namespace engine
{
	namespace error
	{
		void __fastcall throw_error(int code);
		void __fastcall throw_error_if(bool condition, int code);
		void __fastcall throw_error_if(bool condition);

		int handle_exception(const std::exception_ptr&) noexcept;

		class crash_handler
		{
		public:
			static LONG WINAPI handler(LPEXCEPTION_POINTERS) noexcept;
		private:
			static bool _create_dump_file(LPEXCEPTION_POINTERS, LPCWSTR filename) noexcept;
			static bool _create_dump_file(LPEXCEPTION_POINTERS) noexcept;
		};
	}
}
