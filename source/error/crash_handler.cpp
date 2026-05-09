#include "error.hpp"
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")
#include <pathcch.h>
#pragma comment(lib, "pathcch")

namespace engine
{
	namespace error
	{
		LONG WINAPI crash_handler::handler(LPEXCEPTION_POINTERS pointers) noexcept
		{
			_create_dump_file(pointers);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		bool crash_handler::_create_dump_file(LPEXCEPTION_POINTERS pointers, LPCWSTR filename) noexcept
		{
			auto result = false;
			auto file = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file != INVALID_HANDLE_VALUE)
			{
				MINIDUMP_EXCEPTION_INFORMATION dump_info{
					.ThreadId = GetCurrentThreadId(),
					.ExceptionPointers = pointers,
					.ClientPointers = false,
				};
				result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &dump_info, nullptr, nullptr);
				CloseHandle(file);
			}
			return result;
		}

		bool crash_handler::_create_dump_file(LPEXCEPTION_POINTERS pointers) noexcept
		{
			WCHAR filename[MAX_PATH]{};
			if (GetModuleFileNameW(nullptr, filename, ARRAYSIZE(filename)) && SUCCEEDED(PathCchRenameExtension(filename, ARRAYSIZE(filename), L"dmp")))
			{
				return _create_dump_file(pointers, filename);
			}
			return false;
		}
	}
}
