#include "common.hpp"

#include <string>
#include <functional>
#include <memory>
#include <forward_list>

std::string utf8_encode(const std::wstring& input)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0, NULL, NULL);
	std::string output(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &input[0], (int)input.size(), &output[0], size_needed, NULL, NULL);
	return output;
}

std::wstring utf8_decode(const std::string& input)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), NULL, 0);
	std::wstring output(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &input[0], (int)input.size(), &output[0], size_needed);
	return output;
}

namespace engine
{
	int core_window::run(LPCWSTR title, DWORD style, int left, int top, int width, int height) noexcept
	{
		return 0;
	}
}

#include <wrl.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs")
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <d3d11.h>
#pragma comment(lib, "d3d11")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

namespace Engine
{
	inline HRESULT __fastcall GetError() noexcept
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	bool __fastcall SetError(const HRESULT ErrorCode) noexcept
	{
		SetLastError(ErrorCode);
		return SUCCEEDED(GetError());
	}

	template <class C>
	using ComPtr = Microsoft::WRL::ComPtr<C>;

	using D2D1Factory = ComPtr<ID2D1Factory>;
	using D2D1HwndRenderTarget = ComPtr<ID2D1HwndRenderTarget>;
	using D2D1Bitmap = ComPtr<ID2D1Bitmap>;

	using WICImagingFactory = ComPtr<IWICImagingFactory>;
	using WICBitmapDecoder = ComPtr<IWICBitmapDecoder>;
	using WICFormatConverter = ComPtr<IWICFormatConverter>;
	using WICBitmapFrameDecode = ComPtr<IWICBitmapFrameDecode>;
	using WICBitmapSource = ComPtr<IWICBitmapSource>;

	HRESULT LoadBitmapFromFile(WICBitmapSource& BitmapSource, LPCWSTR FileName)
	{
		WICImagingFactory Factory;
		WICBitmapDecoder Decoder;
		WICFormatConverter Converter;
		WICBitmapFrameDecode FrameDecode;

		if (SetError(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(Factory.GetAddressOf()))) &&
			SetError(Factory->CreateDecoderFromFilename(FileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, Decoder.GetAddressOf())) &&
			SetError(Factory->CreateFormatConverter(Converter.GetAddressOf())) &&
			SetError(Decoder->GetFrame(0, FrameDecode.GetAddressOf())) &&
			SetError(Converter->Initialize(FrameDecode.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut)))
		{
			BitmapSource = Converter;
		}

		return GetError();
	}

	using D3D11Device = ComPtr<ID3D11Device>;
	using D3D11DeviceContext = ComPtr<ID3D11DeviceContext>;
	using DXGISwapChain = ComPtr<IDXGISwapChain>;
	using D3D11RenderTargetView = ComPtr<ID3D11RenderTargetView>;
	using D3D11DepthStencilView = ComPtr<ID3D11DepthStencilView>;

	//class D3D11Renderer : public HandlerWrapper
	//{
	//public:
	//	HRESULT Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) override
	//	{
	//		HRESULT Result = S_FALSE;

	//		switch (Message)
	//		{
	//			case WM_PAINT:
	//			{
	//				break;
	//			}

	//			case WM_SIZE:
	//			{

	//				break;
	//			}

	//			case WM_DESTROY:
	//			{

	//				break;
	//			}
	//		}

	//		return Result;
	//	}
	//private:
	//	D2D1Factory _D2DFactory;
	//	D2D1HwndRenderTarget _D2DRenderTarget;
	//	std::function<void(D3D11Renderer&, UINT, UINT)> _D2DHandler;

	//	D3D11Device _D3DDevice;
	//	D3D11DeviceContext _D3DDeviceContext;
	//	DXGISwapChain _D3DSwapChain;
	//	D3D11RenderTargetView _D3DRenderTargetView;
	//	D3D11DepthStencilView _D3DDepthStencilView;
	//	std::function<void(D3D11Renderer&, UINT, UINT)> _D3DHandler;
	//};

	class D2DRenderer
	{
	public:
		void SetHandler(std::function<void(D2DRenderer&, UINT, UINT)> Handle)
		{
			this->DrawHandler = Handle;
		}

		HRESULT CreateBitmapFromFile(D2D1Bitmap& Bitmap, LPCWSTR FileName)
		{
			HRESULT Result = S_OK;

			WICBitmapSource BitmapSource;
			if (SUCCEEDED(Result = LoadBitmapFromFile(BitmapSource, FileName)))
			{
				Result = this->RenderTarget->CreateBitmapFromWicBitmap(BitmapSource.Get(), Bitmap.ReleaseAndGetAddressOf());
			}

			return Result;
		}

		void Clear(float Red, float Green, float Blue, float Alpha = 1.0f)
		{
			this->RenderTarget->Clear(D2D1::ColorF(Red, Green, Blue, Alpha));
		}

		void Draw(const D2D1Bitmap& Bitmap, const D2D1_RECT_F& DestinationRectangle, const D2D1_RECT_F& SourceRectangle, float Opacity = 1.0f)
		{
			this->RenderTarget->DrawBitmap(Bitmap.Get(), DestinationRectangle, Opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, SourceRectangle);
		}

		HRESULT ThisHandler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2)
		{
			switch (Message)
			{
				case WM_PAINT:
				{
					if (this->Factory && this->RenderTarget)
					{
						if (this->DrawHandler)
						{
							this->RenderTarget->BeginDraw();
							const auto Size = this->RenderTarget->GetSize();
							this->DrawHandler(*this, Size.width, Size.height);
							SetError(this->RenderTarget->EndDraw());
						}
					}
					else
					{
						RECT WindowRect{};
						if (SetError(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, this->Factory.ReleaseAndGetAddressOf())) &&
							GetClientRect(Window, &WindowRect))
						{
							SetError(this->Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_HARDWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), D2D1::HwndRenderTargetProperties(Window, D2D1::SizeU(WindowRect.right, WindowRect.bottom)), this->RenderTarget.ReleaseAndGetAddressOf()));
						}
					}

					return GetError();
				}

				case WM_ERASEBKGND:
				{
					return S_OK;
				}

				case WM_SIZE:
				{
					if (this->RenderTarget)
					{
						const auto Size = D2D1::SizeU(GET_X_LPARAM(Param2), GET_Y_LPARAM(Param2));
						if (Size.width && Size.height)
						{
							SetError(this->RenderTarget->Resize(Size));
						}
					}
					return GetError();
				}

				case WM_DESTROY:
				{
					this->RenderTarget.Reset();
					break;
				}
			}

			return S_FALSE;
		}
	private:
		D2D1Factory Factory;
		D2D1HwndRenderTarget RenderTarget;
		std::function<void(D2DRenderer&, UINT, UINT)> DrawHandler;
	};


	class AppHandler : public Core::CApplication
	{
	private:
		bool Handler(HWND Handle, UINT Type, WPARAM Argument1, LPARAM Argument2) noexcept override
		{
			switch (Type)
			{
				case WM_CREATE:
				{
					return true;
				}
			}
			return false;
		}
	};

	HRESULT Core::Main()
	{
		return AppHandler{}.Run(L"Game", WS_VISIBLE | WS_OVERLAPPEDWINDOW, { 100, 100, 640, 480 });
	}
}
