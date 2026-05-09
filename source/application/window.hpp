#pragma once

#include "common/common.hpp"
#include "error/error.hpp"

namespace engine
{
	namespace window
	{
		class window_class
		{
		public:
			window_class(const window_class&) = delete;
			window_class& operator=(const window_class&) = delete;
			explicit window_class(WNDPROC);
			~window_class() noexcept;
		private:
			const HINSTANCE _module;
			LPCWSTR _atom;
			friend class window;
		};

		class window
		{
		public:
			window(const window&) = delete;
			window& operator=(const window&) = delete;
			explicit window(const window_class&, LPCWSTR name, DWORD style, DWORD ex_style, const RECT&, HWND, LPVOID);
			~window() noexcept;
		private:
			const HWND _hwnd;
		};

		class window_handler
		{
		public:
			window_handler& operator=(const window_handler&) = delete;
			int run(LPCWSTR name, DWORD style, DWORD ex_style, const RECT&, HWND);
		protected:
			virtual LRESULT handler(HWND, UINT, WPARAM, LPARAM) noexcept = 0;
		private:
			static LRESULT CALLBACK _static_handler(HWND, UINT, WPARAM, LPARAM) noexcept;
			static LRESULT CALLBACK _this_handler(HWND, UINT, WPARAM, LPARAM) noexcept;
		};
	}
}
