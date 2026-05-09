#pragma once

#include "engine.hpp"
#include <strsafe.h>
#include <wrl.h>
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")
#include <memory>
#include <algorithm>
#include <functional>
#include <format>
#include <future>

template<typename type>
using com_ptr_type = Microsoft::WRL::ComPtr<type>;

class d3d12context
{
public:
	d3d12context(HWND hwnd, const SIZE& size)
	{
		engine::throw_error(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

		const D3D12_COMMAND_QUEUE_DESC queue_desc{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT
		};
		engine::throw_error(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_commandQueue)));

		com_ptr_type<IDXGIFactory4> factory;
		engine::throw_error(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		const DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{
			.Width = static_cast<UINT>(size.cx),
			.Height = static_cast<UINT>(size.cy),
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = {
				.Count = 1
			},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = FrameCount,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		};

		com_ptr_type<IDXGISwapChain1> swap_chain;
		engine::throw_error(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &swap_chain_desc, nullptr, nullptr, &swap_chain));
		engine::throw_error(swap_chain.As(&m_swapChain));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		// 3. Создание дескрипторного хепа для RTV (Render Target View)
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// 4. Создание Command Allocators и Command List
		for (UINT n = 0; n < FrameCount; n++) {
			m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n]));
		}
		m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList));
		m_commandList->Close();

		// 5. Инициализация объектов синхронизации
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		this->resize(size);
	}

	~d3d12context()
	{
		wait_for_gpu();
		CloseHandle(m_fenceEvent);
	}

	void resize(const SIZE& size)
	{
		wait_for_gpu();

		for (UINT n = 0; n < FrameCount; n++) m_renderTargets[n].Reset();

		m_swapChain->ResizeBuffers(FrameCount, static_cast<UINT>(size.cx), static_cast<UINT>(size.cy), DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < FrameCount; n++) {
			m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += m_rtvDescriptorSize;
		}
	}

	void render(float r, float g, float b, std::function<void(com_ptr_type<ID3D12GraphicsCommandList>&)> callback)
	{
		m_commandAllocators[m_frameIndex]->Reset();
		m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

		// Переход ресурса в состояние Render Target
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_commandList->ResourceBarrier(1, &barrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle.ptr += (m_frameIndex * m_rtvDescriptorSize);

		const float clearColor[4] = { r, g, b, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		callback(m_commandList);

		// Переход обратно в Present
		ZeroMemory(&barrier, sizeof(barrier));
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_commandList->ResourceBarrier(1, &barrier);

		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_swapChain->Present(1, 0);

		const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
		m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex]) {
			m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}

		m_fenceValues[m_frameIndex] = currentFenceValue + 1;
	}
private:
	void wait_for_gpu()
	{
		m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]);
		m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		m_fenceValues[m_frameIndex]++;
	}
private:
	static const UINT FrameCount = 2;

	// Основные объекты устройства
	com_ptr_type<ID3D12Device>           m_device;
	com_ptr_type<ID3D12CommandQueue>     m_commandQueue;
	com_ptr_type<IDXGISwapChain3>        m_swapChain;
	com_ptr_type<ID3D12DescriptorHeap>   m_rtvHeap;
	com_ptr_type<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	com_ptr_type<ID3D12GraphicsCommandList> m_commandList;

	// Ресурсы рендер-таргетов
	com_ptr_type<ID3D12Resource>         m_renderTargets[FrameCount];
	UINT m_rtvDescriptorSize = 0;
	UINT m_frameIndex = 0;

	// Синхронизация
	com_ptr_type<ID3D12Fence>            m_fence;
	HANDLE                         m_fenceEvent;
	UINT64                         m_fenceValues[FrameCount];
};
