#include <crtdbg.h>
#include "common/common.hpp"
#include "error/error.hpp"

namespace engine
{
	void throw_error(int code)
	{
		error::throw_error(code);
	}

	void throw_error_if(bool condition, int code)
	{
		error::throw_error_if(condition, code);
	}

	std::wstring unicode(const std::string& string) noexcept
	{
		try
		{
			const auto size = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), static_cast<int>(string.size()), nullptr, 0);
			if (size > 0)
			{
				std::wstring result(size, L'\0');
				MultiByteToWideChar(CP_UTF8, 0, string.c_str(), static_cast<int>(string.size()), result.data(), size);
				return result;
			}
			return {};
		}
		catch (...)
		{
			return {};
		}
	}

	double clock() noexcept
	{
		LARGE_INTEGER frequency, counter;
		if (QueryPerformanceFrequency(&frequency) && QueryPerformanceCounter(&counter))
		{
			return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
		}
		return 0;
	}

	int invoke_main() noexcept
	{
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		SetUnhandledExceptionFilter(error::crash_handler::handler);
		try
		{
			SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
			return engine::main();
		}
		catch (...)
		{
			return error::handle_exception(std::current_exception());
		}
	}
}

int main()
{
	return engine::invoke_main();
}

int wmain()
{
	return engine::invoke_main();
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return engine::invoke_main();
}

_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	return engine::invoke_main();
}
