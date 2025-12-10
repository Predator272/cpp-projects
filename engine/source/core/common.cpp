#include "core.hpp"

namespace Engine
{
	namespace Core
	{
		double QueryPerformanceTimer() noexcept
		{
			LARGE_INTEGER Frequency, Counter;
			if (QueryPerformanceFrequency(&Frequency) && QueryPerformanceCounter(&Counter))
			{
				return ((double)(Counter.QuadPart)) / ((double)(Frequency.QuadPart));
			}
			return 0;
		}

		bool CreateGUIDString(LPWSTR String, DWORD Size) noexcept
		{
			GUID GUIDValue;
			return SUCCEEDED(CoCreateGuid(&GUIDValue)) && StringFromGUID2(GUIDValue, String, (DWORD)Size);
		}
	}
}
