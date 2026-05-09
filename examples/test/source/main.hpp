#pragma once

#include "engine.hpp"

#include <hidsdi.h>
#include <strsafe.h>

#include <wrl.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11")
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")
#include <directxcollision.h>

#include <memory>
#include <vector>

#include "resource/resource.hpp"

std::unique_ptr<wchar_t[]> utf16(const char* string)
{
	constexpr UINT CODE_PAGE = CP_UTF8;
	const auto length = MultiByteToWideChar(CODE_PAGE, 0, string, -1, nullptr, 0);
	auto result = std::make_unique<wchar_t[]>(length);
	MultiByteToWideChar(CODE_PAGE, 0, string, length, result.get(), length);
	return result;
}

class timer_class
{
private:
	double _current, _delta;
public:
	timer_class() noexcept : _current(engine::clock()), _delta(0.0f)
	{
	}
public:
	void update() noexcept
	{
		const auto temp = this->_current;
		this->_current = engine::clock();
		this->_delta = this->_current - temp;
	}

	double current() const noexcept
	{
		return this->_current;
	}

	double delta() const noexcept
	{
		return this->_delta;
	}
};

class camera_class
{
public:
	constexpr camera_class() noexcept : position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), angle(DirectX::XM_PIDIV2), aspect_ratio(1.0f)
	{
	}

	constexpr camera_class(const DirectX::XMFLOAT3A& position, const DirectX::XMFLOAT3A& rotation, float angle, float aspect_ratio) noexcept : position(position), rotation(rotation), angle(angle), aspect_ratio(aspect_ratio)
	{
	}
public:
	DirectX::XMFLOAT4X4A get() const noexcept
	{
		DirectX::XMFLOAT4X4A result{};
		const auto vector = DirectX::XMLoadFloat3A(&this->position);
		const auto matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&this->rotation));
		const auto target = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), matrix);
		const auto up = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), matrix);
		DirectX::XMStoreFloat4x4A(&result, DirectX::XMMatrixMultiply(DirectX::XMMatrixLookAtLH(vector, DirectX::XMVectorAdd(vector, target), up), DirectX::XMMatrixPerspectiveFovLH(this->angle, this->aspect_ratio, 0.01f, 100000.0f)));
		return result;
	}

	DirectX::XMFLOAT4X4A transform(const DirectX::XMFLOAT3A& position = DirectX::XMFLOAT3A(0.0f, 0.0f, 0.0f), const DirectX::XMFLOAT3A& rotation = DirectX::XMFLOAT3A(0.0f, 0.0f, 0.0f), const DirectX::XMFLOAT3A& scale = DirectX::XMFLOAT3A(1.0f, 1.0f, 1.0f)) const noexcept
	{
		DirectX::XMFLOAT4X4A result;
		const auto matrix_position = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3A(&position));
		const auto matrix_rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rotation));
		const auto matrix_scale = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3A(&scale));
		DirectX::XMStoreFloat4x4A(&result, DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(matrix_position, matrix_rotation), matrix_scale));
		return result;
	}
public:
	DirectX::XMFLOAT3A position, rotation;
	float angle, aspect_ratio;
};



namespace engine
{
	class raw_input
	{
	public:
		bool handle(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
		{
			switch (message)
			{
				case WM_CREATE:
				{
					const RAWINPUTDEVICE raw_input_device[]{
						{ HID_USAGE_PAGE_GENERIC, 0, RIDEV_PAGEONLY | RIDEV_INPUTSINK, window },
					};
					throw_last_code(RegisterRawInputDevices(raw_input_device, ARRAYSIZE(raw_input_device), sizeof(raw_input_device[0])));
					return true;
				}

				case WM_INPUT:
				{
					RAWINPUT data{};
					UINT size = sizeof(data);
					if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &data, &size, sizeof(data.header)) != ((UINT)-1))
					{
						this->handle_raw_input(data);
					}
					return true;
				}
			}
			return false;
		}
	public:
		virtual void handle_raw_input(const RAWINPUT& input) = 0;
	};

	using Microsoft::WRL::ComPtr;

	ComPtr<ID3DBlob> load_from_resource(UINT resource_id) noexcept
	{
		ComPtr<ID3DBlob> result;
		auto module = GetModuleHandleW(nullptr);
		auto resource = FindResource(module, MAKEINTRESOURCE(resource_id), RT_RCDATA);
		throw_last_code(resource);
		auto memory = LoadResource(module, resource);
		throw_last_code(memory);
		throw_code(D3DCreateBlob(SizeofResource(module, resource), result.GetAddressOf()));
		CopyMemory(result->GetBufferPointer(), LockResource(memory), result->GetBufferSize());
		return result;
	}

	ComPtr<ID3DBlob> compile_shader(LPCSTR target, ComPtr<ID3DBlob> source) noexcept
	{
		ComPtr<ID3DBlob> shader, error;
		const auto result = D3DCompile(source->GetBufferPointer(), source->GetBufferSize(), nullptr, nullptr, nullptr, "main", target, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, shader.GetAddressOf(), error.GetAddressOf());
		if (FAILED(result))
		{
#ifdef _DEBUG
			OutputDebugStringA((LPCSTR)(error->GetBufferPointer()));
#endif
		}
		throw_code(result);
		return shader;
	}

	ComPtr<ID3DBlob> compile_shader(LPCSTR target, UINT resource_id) noexcept
	{
		return compile_shader(target, load_from_resource(resource_id));
	}

	class d3d11object
	{
	public:
		using d3d11device = ComPtr<ID3D11Device>;
		using d3d11context = ComPtr<ID3D11DeviceContext>;
	private:
		d3d11device _device;
		d3d11context _device_context;
	public:
		d3d11object() noexcept : d3d11object(nullptr, nullptr)
		{
		}

		d3d11object(const d3d11object& object) noexcept : d3d11object(object._device, object._device_context)
		{
		}

		d3d11object(const d3d11device& device, const d3d11context& context) noexcept : _device(device), _device_context(context)
		{
		}
	public:
		operator bool() const noexcept
		{
			return (this->_device && this->_device_context);
		}

		d3d11device& device() noexcept
		{
			return this->_device;
		}

		const d3d11device& device() const noexcept
		{
			return this->_device;
		}

		d3d11context& context() noexcept
		{
			return this->_device_context;
		}

		const d3d11context& context() const noexcept
		{
			return this->_device_context;
		}
	};

	class d3d11renderer : public d3d11object
	{
	public:
		ComPtr<IDXGISwapChain> _swap_chain;
		ComPtr<ID3D11RenderTargetView> _render_target_view;
		ComPtr<ID3D11DepthStencilView> _depth_stencil_view;
	public:
		bool handle(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
		{
			switch (message)
			{
				case WM_CREATE:
				{
					throw_code(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, this->device().GetAddressOf(), nullptr, this->context().GetAddressOf()));
					this->_create_swap_chain(window, 0, 0, 1);
					return true;
				}

				case WM_SIZE:
				{
					const UINT width = GET_X_LPARAM(lparam), height = GET_Y_LPARAM(lparam);
					this->_create_swap_chain(window, width, height, 0);
					return true;
				}

				case WM_PAINT:
				{
					const float clear_color[4]{ 0, 0, 0, 0 };
					this->context()->ClearRenderTargetView(this->_render_target_view.Get(), clear_color);
					this->context()->ClearDepthStencilView(this->_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
					RECT rect{};
					throw_last_code(GetClientRect(window, &rect));
					this->handle_d3drenderer(window, rect.right - rect.left, rect.bottom - rect.top);
					throw_code(this->_swap_chain->Present(0, 0));
					return true;
				}

				case WM_ERASEBKGND:
				{
					return true;
				}
			}
			return false;
		}
	public:
		virtual void handle_d3drenderer(HWND window, UINT width, UINT height) = 0;
	private:
		void _create_swap_chain(HWND window, UINT width, UINT height, UINT samples)
		{
			DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

			if (this->_swap_chain && SUCCEEDED(this->_swap_chain->GetDesc(&swap_chain_desc)))
			{
				if (window)
				{
					swap_chain_desc.OutputWindow = window;
				}

				if (width)
				{
					swap_chain_desc.BufferDesc.Width = width;
				}

				if (height)
				{
					swap_chain_desc.BufferDesc.Height = height;
				}

				if (samples)
				{
					swap_chain_desc.SampleDesc.Count = samples;
				}
			}
			else
			{
				swap_chain_desc.OutputWindow = window;
				swap_chain_desc.BufferDesc.Width = width;
				swap_chain_desc.BufferDesc.Height = height;
				swap_chain_desc.SampleDesc.Count = samples;
			}

			swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.BufferCount = 1;
			swap_chain_desc.Windowed = true;

			ComPtr<IDXGIDevice> device;
			ComPtr<IDXGIAdapter> adapter;
			ComPtr<IDXGIFactory> factory;
			throw_code(this->device().As(&device));
			throw_code(device->GetAdapter(adapter.GetAddressOf()));
			throw_code(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())));
			throw_code(factory->CreateSwapChain(this->device().Get(), &swap_chain_desc, this->_swap_chain.ReleaseAndGetAddressOf()));
			throw_code(factory->MakeWindowAssociation(swap_chain_desc.OutputWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

			ComPtr<ID3D11Texture2D> back_buffer, depth_stencil_view_buffer;
			throw_code(this->_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())));
			throw_code(this->device()->CreateRenderTargetView(back_buffer.Get(), nullptr, this->_render_target_view.ReleaseAndGetAddressOf()));

			D3D11_TEXTURE2D_DESC back_buffer_desc{};
			back_buffer->GetDesc(&back_buffer_desc);
			back_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			back_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			throw_code(this->device()->CreateTexture2D(&back_buffer_desc, nullptr, depth_stencil_view_buffer.GetAddressOf()));
			throw_code(this->device()->CreateDepthStencilView(depth_stencil_view_buffer.Get(), nullptr, this->_depth_stencil_view.ReleaseAndGetAddressOf()));
			this->context()->OMSetRenderTargets(1, this->_render_target_view.GetAddressOf(), this->_depth_stencil_view.Get());

			const CD3D11_VIEWPORT view_port(0.0f, 0.0f, (float)(back_buffer_desc.Width), (float)(back_buffer_desc.Height));
			this->context()->RSSetViewports(1, &view_port);
		}
	};

	class d3d11material : public d3d11object
	{
	private:
		ComPtr<ID3D11InputLayout> _input_layout;

		ComPtr<ID3D11VertexShader> _vertex_shader;
		ComPtr<ID3D11PixelShader> _pixel_shader;

		ComPtr<ID3D11Buffer> _shader_constant_buffer[2][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
		ComPtr<ID3D11ShaderResourceView> _shader_resource_view[2][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	public:
		d3d11material(const d3d11object& object) noexcept : d3d11object(object)
		{
		}
	private:
		ComPtr<ID3D11Buffer>& _get_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index) noexcept
		{
			return this->_shader_constant_buffer[type % ARRAYSIZE(this->_shader_constant_buffer)][index % ARRAYSIZE(this->_shader_constant_buffer[0])];
		}

		ComPtr<ID3D11ShaderResourceView>& _get_shader_resource_view(D3D11_SHADER_VERSION_TYPE type, UINT index) noexcept
		{
			return this->_shader_resource_view[type % ARRAYSIZE(this->_shader_resource_view)][index % ARRAYSIZE(this->_shader_resource_view[0])];
		}
	public:
		void create_shader(D3D11_SHADER_VERSION_TYPE type, ComPtr<ID3DBlob> code) noexcept
		{
			if (code)
			{
				const auto data = code->GetBufferPointer();
				const auto size = code->GetBufferSize();
				switch (type)
				{
					case D3D11_SHVER_PIXEL_SHADER:
					{
						throw_code(this->device()->CreatePixelShader(data, size, nullptr, this->_pixel_shader.ReleaseAndGetAddressOf()));
						return;
					}

					case D3D11_SHVER_VERTEX_SHADER:
					{
						Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
						throw_code(D3DReflect(data, size, IID_PPV_ARGS(reflection.GetAddressOf())));

						D3D11_SHADER_DESC shader_desc{};
						throw_code(reflection->GetDesc(&shader_desc));

						D3D11_INPUT_ELEMENT_DESC shader_input_element[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT]{};
						const UINT shader_input_element_count = min(ARRAYSIZE(shader_input_element), shader_desc.InputParameters);
						for (UINT i = 0; i != shader_input_element_count; i++)
						{
							D3D11_SIGNATURE_PARAMETER_DESC signature{};
							throw_code(reflection->GetInputParameterDesc(i, &signature));

							constexpr DXGI_FORMAT format[][5]{
								{ DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32G32_TYPELESS, DXGI_FORMAT_R32G32B32_TYPELESS, DXGI_FORMAT_R32G32B32A32_TYPELESS },
								{ DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32B32_UINT, DXGI_FORMAT_R32G32B32A32_UINT },
								{ DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32B32_SINT, DXGI_FORMAT_R32G32B32A32_SINT },
								{ DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT },
							};

							shader_input_element[i].SemanticName = signature.SemanticName;
							shader_input_element[i].SemanticIndex = signature.SemanticIndex;
							shader_input_element[i].Format = format[signature.ComponentType][__popcnt(signature.Mask & 0b1111)];
							shader_input_element[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
						}

						throw_code(this->device()->CreateInputLayout(shader_input_element, shader_input_element_count, data, size, this->_input_layout.ReleaseAndGetAddressOf()));
						throw_code(this->device()->CreateVertexShader(data, size, nullptr, this->_vertex_shader.ReleaseAndGetAddressOf()));
						return;
					}
				}
			}
			throw_code(E_INVALIDARG);
		}

		void create_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, UINT size) noexcept
		{
			ComPtr<ID3D11Buffer> buffer;
			const CD3D11_BUFFER_DESC buffer_desc(size, D3D11_BIND_CONSTANT_BUFFER);
			throw_code(this->device()->CreateBuffer(&buffer_desc, nullptr, this->_get_constant_buffer(type, index).ReleaseAndGetAddressOf()));
		}

		void create_texture(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data, UINT width, UINT height) noexcept
		{
			ComPtr<ID3D11Texture2D> texture;
			const CD3D11_TEXTURE2D_DESC texture_desc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

			const D3D11_SUBRESOURCE_DATA texture_data{
				.pSysMem = data,
				.SysMemPitch = width * sizeof(UINT),
			};

			throw_code(this->device()->CreateTexture2D(&texture_desc, &texture_data, texture.GetAddressOf()));
			throw_code(this->device()->CreateShaderResourceView(texture.Get(), nullptr, this->_get_shader_resource_view(type, index).ReleaseAndGetAddressOf()));
		}

		void create_structured_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data, UINT count, UINT size) noexcept
		{
			ComPtr<ID3D11Buffer> buffer;
			const CD3D11_BUFFER_DESC buffer_desc(count * size, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, size);

			const D3D11_SUBRESOURCE_DATA buffer_data{
				.pSysMem = data,
				.SysMemPitch = size,
			};

			throw_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, buffer.GetAddressOf()));
			throw_code(this->device()->CreateShaderResourceView(buffer.Get(), nullptr, this->_get_shader_resource_view(type, index).ReleaseAndGetAddressOf()));
		}

		void update_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data) noexcept
		{
			this->context()->UpdateSubresource(this->_get_constant_buffer(type, index).Get(), 0, nullptr, data, 0, 0);
		}

		void use(D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) const noexcept
		{
			this->context()->IASetPrimitiveTopology(topology);

			this->context()->IASetInputLayout(this->_input_layout.Get());

			this->context()->VSSetShader(this->_vertex_shader.Get(), nullptr, 0);
			this->context()->VSSetConstantBuffers(0, ARRAYSIZE(this->_shader_constant_buffer[D3D11_SHVER_VERTEX_SHADER]), this->_shader_constant_buffer[D3D11_SHVER_VERTEX_SHADER]->GetAddressOf());
			this->context()->VSSetShaderResources(0, ARRAYSIZE(this->_shader_resource_view[D3D11_SHVER_VERTEX_SHADER]), this->_shader_resource_view[D3D11_SHVER_VERTEX_SHADER]->GetAddressOf());

			this->context()->PSSetShader(this->_pixel_shader.Get(), nullptr, 0);
			this->context()->PSSetConstantBuffers(0, ARRAYSIZE(this->_shader_constant_buffer[D3D11_SHVER_PIXEL_SHADER]), this->_shader_constant_buffer[D3D11_SHVER_PIXEL_SHADER]->GetAddressOf());
			this->context()->PSSetShaderResources(0, ARRAYSIZE(this->_shader_resource_view[D3D11_SHVER_PIXEL_SHADER]), this->_shader_resource_view[D3D11_SHVER_PIXEL_SHADER]->GetAddressOf());
		}
	};

	class d3d11mesh : public d3d11object
	{
	private:
		ComPtr<ID3D11Buffer> _vertex_buffer, _index_buffer;
	public:
		d3d11mesh(const d3d11object& object, LPCVOID vertex_data, UINT vertex_data_size, LPCVOID index_data, UINT index_data_size) : d3d11object(object)
		{
			D3D11_BUFFER_DESC buffer_desc{};
			D3D11_SUBRESOURCE_DATA buffer_data{};

			buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;

			buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			buffer_desc.ByteWidth = vertex_data_size;
			buffer_data.pSysMem = vertex_data;
			throw_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, this->_vertex_buffer.GetAddressOf()));

			buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			buffer_desc.ByteWidth = index_data_size;
			buffer_data.pSysMem = index_data;
			throw_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, this->_index_buffer.GetAddressOf()));
		}
	public:
		void draw(UINT instance_count = 1) const noexcept
		{
			static constexpr UINT strides[1]{ sizeof(float) * 8 }, offsets[1]{};
			this->context()->IASetVertexBuffers(0, 1, this->_vertex_buffer.GetAddressOf(), strides, offsets);
			this->context()->IASetIndexBuffer(this->_index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

			D3D11_BUFFER_DESC buffer_desc{};
			this->_index_buffer->GetDesc(&buffer_desc);
			this->context()->DrawIndexedInstanced(buffer_desc.ByteWidth / sizeof(UINT), instance_count, 0, 0, 0);
		}
	};
}
