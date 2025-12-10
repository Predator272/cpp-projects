#include "common.hpp"
#include <strsafe.h>
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

EXTERN_C __declspec(dllexport) const int NvOptimusEnablement = 1;
EXTERN_C __declspec(dllexport) const int AmdPowerXpressRequestHighPerformance = 1;

namespace engine
{
	double clock() noexcept
	{
		LARGE_INTEGER frequency, counter;
		if (QueryPerformanceFrequency(&frequency) && QueryPerformanceCounter(&counter))
		{
			return ((double)(counter.QuadPart)) / ((double)(frequency.QuadPart));
		}
		return 0;
	}

	bool create_guid_string(const std::span<WCHAR>& string) noexcept
	{
		GUID value{};
		return SUCCEEDED(CoCreateGuid(&value)) && StringFromGUID2(value, string.data(), (int)string.size());
	}

	void show_error(LPCWSTR title, LPCWSTR format, ...) noexcept
	{
		va_list list;
		va_start(list, format);
		std::array<WCHAR, 0x200> buffer{};
		if (SUCCEEDED(StringCchVPrintfW(buffer.data(), buffer.size(), format, list)))
		{
			MessageBoxW(HWND_DESKTOP, buffer.data(), title, MB_OK | MB_ICONERROR | MB_SERVICE_NOTIFICATION);
		}
		va_end(list);
	}
}
