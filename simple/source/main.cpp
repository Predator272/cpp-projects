#include "main.hpp"

#include <wrl.h>
#include <dxgi1_4.h>
#pragma comment(lib, "dxgi")
#include <d3d12.h>
#pragma comment(lib, "d3d12")

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class CApplication : public Engine::IApplication
{
private:
	bool Handler(HWND Window, UINT Message, WPARAM Param1, LPARAM Param2) noexcept override
	{
		switch (Message)
		{
			case WM_CREATE:
			{
				ComPtr<IDXGIFactory4> Factory;
				ComPtr<ID3D12Device> Device;
				if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(Factory.GetAddressOf()))))
				{
					ComPtr<IDXGIAdapter1> Adapter;
					for (UINT Index = 0; Factory->EnumAdapters1(Index, &Adapter) != DXGI_ERROR_NOT_FOUND; Index++)
					{
						DXGI_ADAPTER_DESC1 AdapterDesc;
						Adapter->GetDesc1(&AdapterDesc);

						OutputDebugStringW(AdapterDesc.Description);
					}

					if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(Device.GetAddressOf()))))
					{

					}
				}

				return true;
			}

			case WM_PAINT:
			{


				return true;
			}
		}
		return false;
	}
};

_Use_decl_annotations_
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	return CApplication{}.Run(L"Simple", WS_VISIBLE | WS_OVERLAPPEDWINDOW, { 100, 100, 640, 480 });
}
