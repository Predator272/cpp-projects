#pragma once

#include <windows.h>
#include <windowsx.h>
#include <string>
#include <optional>

namespace engine
{
	int main();

	double clock() noexcept;
	void throw_error(int);
	void throw_error_if(bool, int);

	class application
	{
	public:
		struct config_t
		{
			std::string title;
			std::uint32_t style;
			struct { std::int32_t x, y; } position;
			struct { std::int32_t x, y; } size;
		};

		class window
		{
		public:
			virtual HWND hwnd() const noexcept = 0;
			virtual SIZE size() const noexcept = 0;
			virtual void title(const std::string&) const noexcept = 0;
			virtual void close() const noexcept = 0;
		};

		class event
		{
		public:
			virtual const window& window() const noexcept = 0;
			virtual UINT type() const noexcept = 0;
			virtual WPARAM wparam() const noexcept = 0;
			virtual LPARAM lparam() const noexcept = 0;
		};
	public:
		int run(const config_t&);
	public:
		virtual std::optional<LRESULT> handler(const event&) = 0;
	};
}
