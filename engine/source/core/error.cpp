#include "core.hpp"

#include <strsafe.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp")
#include <pathcch.h>
#pragma comment(lib, "pathcch")

namespace Engine
{
	namespace Core
	{
		static void ShowError(LPCWSTR Format, ...) noexcept
		{
			va_list Arguments;
			va_start(Arguments, Format);
			WCHAR Buffer[0x200]{};
			if (SUCCEEDED(StringCchVPrintfW(Buffer, ARRAYSIZE(Buffer), Format, Arguments)))
			{
				MessageBoxW(HWND_DESKTOP, Buffer, nullptr, MB_OK | MB_ICONERROR | MB_SERVICE_NOTIFICATION);
			}
			va_end(Arguments);
		}

		static bool CreateDumpFile(LPEXCEPTION_POINTERS ExceptionPointers, LPCWSTR FileName) noexcept
		{
			bool Result = false;
			auto OutputFile = CreateFileW(FileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (OutputFile != INVALID_HANDLE_VALUE)
			{
				MINIDUMP_EXCEPTION_INFORMATION MiniDumpInfo{
					.ThreadId = GetCurrentThreadId(),
					.ExceptionPointers = ExceptionPointers,
					.ClientPointers = false
				};
				Result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), OutputFile, MiniDumpNormal, &MiniDumpInfo, nullptr, nullptr);
				CloseHandle(OutputFile);
			}
			return Result;
		}

		static bool CreateDumpFile(LPEXCEPTION_POINTERS ExceptionPointers) noexcept
		{
			WCHAR FileName[MAX_PATH]{};
			if (GetModuleFileNameW(nullptr, FileName, ARRAYSIZE(FileName)) && SUCCEEDED(PathCchRenameExtension(FileName, ARRAYSIZE(FileName), L"dmp")))
			{
				return CreateDumpFile(ExceptionPointers, FileName);
			}
			return false;
		}

		static LONG WINAPI Handler(LPEXCEPTION_POINTERS ExceptionPointers)
		{
			CreateDumpFile(ExceptionPointers);
			ShowError(L"0x%p: fatal error 0x%08X", ExceptionPointers->ExceptionRecord->ExceptionCode, ExceptionPointers->ExceptionRecord->ExceptionAddress);
			ExitProcess(ExceptionPointers->ExceptionRecord->ExceptionCode);
			return EXCEPTION_EXECUTE_HANDLER;
		}

		CCrashHandler::CCrashHandler() noexcept : _Handler{ AddVectoredExceptionHandler(0, Handler) }
		{
		}

		CCrashHandler::~CCrashHandler() noexcept
		{
			RemoveVectoredExceptionHandler(this->_Handler);
		}
	}
}
