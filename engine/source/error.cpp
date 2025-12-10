#include "common.hpp"
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")
#include <pathcch.h>
#pragma comment(lib, "pathcch")
#include <exception>

namespace engine
{
	void throw_code(const int code)
	{
		error::throw_code(code);
	}

	void throw_last_code(const bool expression)
	{
		error::throw_last_code(expression);
	}

	constexpr error::error(const int code) noexcept : _code(code)
	{
	}

	int error::code() const noexcept
	{
		return this->_code;
	}

	void safe_throw_error(const error& object)
	{
		if (std::uncaught_exceptions() == 0 && object.code() != 0)
		{
			throw object;
		}
	}

	void error::throw_code(const int code)
	{
		safe_throw_error(error(code));
	}

	void error::throw_last_code(const bool expression)
	{
		safe_throw_error(error((!expression) ? ((int)GetLastError()) : 0));
	}

	static bool create_dump_file(LPEXCEPTION_POINTERS pointers, LPCWSTR filename) noexcept
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

	static bool create_dump_file(LPEXCEPTION_POINTERS pointers) noexcept
	{
		std::array<WCHAR, MAX_PATH> filename{};
		if (GetModuleFileNameW(nullptr, filename.data(), (DWORD)filename.size()) && SUCCEEDED(PathCchRenameExtension(filename.data(), filename.size(), L"dmp")))
		{
			return create_dump_file(pointers, filename.data());
		}
		return false;
	}

	static LONG WINAPI crash_handler(LPEXCEPTION_POINTERS pointers)
	{
		create_dump_file(pointers);
		show_error(nullptr, L"0x%p: fatal error 0x%08X", pointers->ExceptionRecord->ExceptionCode, pointers->ExceptionRecord->ExceptionAddress);
		return EXCEPTION_EXECUTE_HANDLER;
	}
}
