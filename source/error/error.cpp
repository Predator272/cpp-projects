#include "error.hpp"

namespace engine
{
	namespace error
	{
		void __fastcall throw_error(int code)
		{
			if (code && std::uncaught_exceptions() == 0)
			{
				throw std::system_error(code, std::system_category());
			}
		}

		void __fastcall throw_error_if(bool condition, int code)
		{
			if (condition)
			{
				throw_error(code);
			}
		}

		void __fastcall throw_error_if(bool condition)
		{
			throw_error_if(condition, static_cast<int>(HRESULT_FROM_WIN32(GetLastError())));
		}

		int show_error(const std::system_error& error) noexcept
		{
			const auto string = std::format("Code: 0x{:08X}\r\n{}", static_cast<uint32_t>(error.code().value()), error.what());
			MessageBoxW(nullptr, unicode(string).c_str(), nullptr, MB_OK | MB_ICONERROR | MB_SERVICE_NOTIFICATION);
			return error.code().value();
		}

		int handle_exception(const std::exception_ptr& exception) noexcept
		{
			try
			{
				if (exception)
				{
					std::rethrow_exception(exception);
				}
			}
			catch (const std::system_error& error)
			{
				return show_error(error);
			}
			catch (const std::exception& error)
			{
				return show_error(std::system_error(static_cast<int>(E_UNEXPECTED), std::system_category(), error.what()));
			}
			catch (...)
			{
				return show_error(std::system_error(static_cast<int>(E_UNEXPECTED), std::system_category()));
			}
			return 0;
		}
	}
}
