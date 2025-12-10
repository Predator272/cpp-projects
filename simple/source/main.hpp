#pragma once

#include "engine.hpp"

#include <tchar.h>
#include <hidsdi.h>
#include <strsafe.h>

#include <wrl.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")


namespace Engine
{
	template <class _Class>
	using ComPtr = Microsoft::WRL::ComPtr<_Class>;

	using D2D1Factory = ComPtr<ID2D1Factory>;
	using DWriteFactory = ComPtr<IDWriteFactory>;
	using D2D1HwndRenderTarget = ComPtr<ID2D1HwndRenderTarget>;

	using DWriteTextFormat = ComPtr<IDWriteTextFormat>;

	using D2D1Brush = ComPtr<ID2D1Brush>;
	using D2D1SolidColorBrush = ComPtr<ID2D1SolidColorBrush>;
	using D2D1Bitmap = ComPtr<ID2D1Bitmap>;


	class BasicHandler
	{
	public:
		void Call(bool& Return, HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept
		{
			Return |= this->InternalHandler(Handle, Type, Argument1, Argument2);
		}
	protected:
		virtual bool InternalHandler(HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept = 0;
	};

	class RawInput : public BasicHandler
	{
	public:
		virtual void HandlerMouse(HWND Handle, int DeltaX, int DeltaY, int Scroll, USHORT Flags) noexcept
		{
		}

		virtual void HandlerKeyboard(HWND Handle, BYTE Code, bool State) noexcept
		{
		}
	private:
		bool InternalHandler(HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept override
		{
			switch (Type)
			{
				case WM_CREATE:
				{
					const RAWINPUTDEVICE RawInputDevices[]{
						{ HID_USAGE_PAGE_GENERIC, 0, RIDEV_PAGEONLY | RIDEV_INPUTSINK, Handle },
					};
					RegisterRawInputDevices(RawInputDevices, ARRAYSIZE(RawInputDevices), sizeof(RawInputDevices[0]));
					return true;
				}

				case WM_INPUT:
				{
					RAWINPUT Data{};
					UINT Size = sizeof(Data);
					if (GetRawInputData((HRAWINPUT)Argument2, RID_INPUT, &Data, &Size, sizeof(Data.header)) != ((UINT)-1))
					{
						switch (Data.header.dwType)
						{
							case RIM_TYPEMOUSE:
							{
								const auto Scroll = ((Data.data.mouse.usButtonFlags & RI_MOUSE_WHEEL) || (Data.data.mouse.usButtonFlags & RI_MOUSE_HWHEEL)) ? Data.data.mouse.usButtonData / WHEEL_DELTA : 0;
								this->HandlerMouse(Handle, Data.data.mouse.lLastX, Data.data.mouse.lLastY, Scroll, Data.data.mouse.usButtonFlags);
								break;
							}

							case RIM_TYPEKEYBOARD:
							{
								this->HandlerKeyboard(Handle, (BYTE)Data.data.keyboard.VKey, Data.data.keyboard.Flags == RI_KEY_MAKE);
								break;
							}
						}
					}
					return true;
				}
			}
			return false;
		}
	};

	class D2DRenderer : public BasicHandler
	{
	public:
		virtual void Handler(HWND Handle, const D2D1_SIZE_F& Size) noexcept
		{
		}
	protected:
		HRESULT Create(DWriteTextFormat& Object, LPCWSTR FontFamily, DWRITE_FONT_WEIGHT Weight, DWRITE_FONT_STYLE Style, DWRITE_FONT_STRETCH Stretch, float Size) const noexcept
		{
			return this->_WriteFactory->CreateTextFormat(FontFamily, nullptr, Weight, Style, Stretch, Size, L"", Object.ReleaseAndGetAddressOf());
		}

		HRESULT Create(D2D1SolidColorBrush& Object, const D2D1_COLOR_F& Color) const noexcept
		{
			return this->_RenderTarget->CreateSolidColorBrush(Color, Object.ReleaseAndGetAddressOf());
		}

		void Draw(const DWriteTextFormat& TextFormat, const D2D1Brush& Brush, LPCWSTR _String, const D2D1_RECT_F& Rect) const noexcept
		{
			this->_RenderTarget->DrawTextW(_String, lstrlenW(_String), TextFormat.Get(), Rect, Brush.Get());
		}
	private:
		bool InternalHandler(HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept override
		{
			switch (Type)
			{
				case WM_CREATE:
				{
					RECT WindowRect{};
					if (SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, this->_Factory.ReleaseAndGetAddressOf())) &&
						SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)this->_WriteFactory.ReleaseAndGetAddressOf())) &&
						GetClientRect(Handle, &WindowRect))
					{
						this->_Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat()), D2D1::HwndRenderTargetProperties(Handle, D2D1::SizeU(WindowRect.right, WindowRect.bottom)), this->_RenderTarget.ReleaseAndGetAddressOf());
					}
					return true;
				}

				case WM_PAINT:
				{
					if (this->_Factory && this->_WriteFactory && this->_RenderTarget)
					{
						this->_RenderTarget->BeginDraw();
						this->_RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
						this->Handler(Handle, this->_RenderTarget->GetSize());
						this->_RenderTarget->EndDraw();
					}
					return true;
				}

				case WM_ERASEBKGND:
				{
					return true;
				}

				case WM_SIZE:
				{
					if (this->_RenderTarget)
					{
						const auto Size = D2D1::SizeU(GET_X_LPARAM(Argument2), GET_Y_LPARAM(Argument2));
						if (Size.width && Size.height)
						{
							this->_RenderTarget->Resize(Size);
						}
					}
					return true;
				}

				case WM_DESTROY:
				{
					this->_RenderTarget.Reset();
					return true;
				}
			}
			return false;
		}
	private:
		D2D1Factory _Factory;
		DWriteFactory _WriteFactory;
		D2D1HwndRenderTarget _RenderTarget;
	};
}
