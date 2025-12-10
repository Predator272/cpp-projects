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

std::unique_ptr<wchar_t[]> utf16(const char* string) noexcept(false)
{
	constexpr UINT CODE_PAGE = CP_UTF8;
	const auto length = MultiByteToWideChar(CODE_PAGE, 0, string, -1, nullptr, 0);
	auto result = std::make_unique<wchar_t[]>(length);
	MultiByteToWideChar(CODE_PAGE, 0, string, length, result.get(), length);
	return result;
}

class timer_class
{
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

	float current() const noexcept
	{
		return this->_current;
	}

	float delta() const noexcept
	{
		return this->_delta;
	}
private:
	float _current, _delta;
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

class raw_input_class
{
public:
	static HRESULT register_input(HWND window) noexcept
	{
		const RAWINPUTDEVICE raw_input_device[]{
			{ HID_USAGE_PAGE_GENERIC, 0, RIDEV_PAGEONLY | RIDEV_INPUTSINK, window },
		};
		RegisterRawInputDevices(raw_input_device, ARRAYSIZE(raw_input_device), sizeof(raw_input_device[0]));
		return HRESULT_FROM_WIN32(GetLastError());
	}

	static bool get_input(LPARAM lparam, RAWINPUT& data) noexcept
	{
		UINT size = sizeof(data);
		return GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, &data, &size, sizeof(data.header)) != static_cast<UINT>(-1);
	}
};

namespace engine
{
	namespace directx11
	{
		static Microsoft::WRL::ComPtr<ID3DBlob> load_raw_data(WORD id) noexcept
		{
			Microsoft::WRL::ComPtr<ID3DBlob> result;

			auto module = GetModuleHandleW(nullptr);
			if (auto resource = FindResourceW(module, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10)))
			{
				if (auto memory = LoadResource(module, resource))
				{
					if (SUCCEEDED(D3DCreateBlob(SizeofResource(module, resource), result.GetAddressOf())))
					{
						CopyMemory(result->GetBufferPointer(), LockResource(memory), result->GetBufferSize());
					}
				}
			}

			return result;
		}

		static Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(LPCSTR target, Microsoft::WRL::ComPtr<ID3DBlob> source) noexcept
		{
			Microsoft::WRL::ComPtr<ID3DBlob> shader, error;
			if (source)
			{
				if (FAILED(D3DCompile(source->GetBufferPointer(), source->GetBufferSize(), nullptr, nullptr, nullptr, "main", target, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, shader.GetAddressOf(), error.GetAddressOf())))
				{
					OutputDebugStringA(static_cast<LPCSTR>(error->GetBufferPointer()));
				}
			}
			return shader;
		}

		static Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(LPCSTR target, WORD id) noexcept
		{
			return compile_shader(target, load_raw_data(id));
		}


		class base_context_class
		{
		public:
			base_context_class()
			{
				failed_code(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, this->_device.GetAddressOf(), nullptr, this->_context.GetAddressOf()));
			}

			base_context_class(const base_context_class& other) noexcept : _device(other._device), _context(other._context)
			{
			}
		protected:
			const Microsoft::WRL::ComPtr<ID3D11Device>& device() const noexcept
			{
				return this->_device;
			}

			const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context() const noexcept
			{
				return this->_context;
			}
		private:
			Microsoft::WRL::ComPtr<ID3D11Device> _device;
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
		};


		class context_class : public base_context_class
		{
		public:
			context_class(HWND window, UINT width, UINT height, UINT samples)
			{
				failed_code(this->create_swap_chain(window, width, height, samples));
			}
		public:
			void clear(float r, float g, float b, float a) const noexcept
			{
				const float clear_color[4]{ r, g, b, a };
				this->context()->ClearRenderTargetView(this->_render_target_view.Get(), clear_color);
				this->context()->ClearDepthStencilView(this->_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			}

			int present(UINT sync_interval = 0) const noexcept
			{
				return this->_swap_chain->Present(sync_interval, 0);
			}

			int create_swap_chain(HWND window, UINT width, UINT height, UINT samples) noexcept
			{
				try
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

					Microsoft::WRL::ComPtr<IDXGIDevice> device;
					Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
					Microsoft::WRL::ComPtr<IDXGIFactory> factory;
					failed_code(this->device().As(&device));
					failed_code(device->GetAdapter(adapter.GetAddressOf()));
					failed_code(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())));
					failed_code(factory->CreateSwapChain(this->device().Get(), &swap_chain_desc, this->_swap_chain.ReleaseAndGetAddressOf()));
					failed_code(factory->MakeWindowAssociation(swap_chain_desc.OutputWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

					Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer, depth_stencil_view_buffer;
					failed_code(this->_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())));
					failed_code(this->device()->CreateRenderTargetView(back_buffer.Get(), nullptr, this->_render_target_view.ReleaseAndGetAddressOf()));

					D3D11_TEXTURE2D_DESC back_buffer_desc{};
					back_buffer->GetDesc(&back_buffer_desc);
					back_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					back_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
					failed_code(this->device()->CreateTexture2D(&back_buffer_desc, nullptr, depth_stencil_view_buffer.GetAddressOf()));
					failed_code(this->device()->CreateDepthStencilView(depth_stencil_view_buffer.Get(), nullptr, this->_depth_stencil_view.ReleaseAndGetAddressOf()));
					this->context()->OMSetRenderTargets(1, this->_render_target_view.GetAddressOf(), this->_depth_stencil_view.Get());

					const CD3D11_VIEWPORT view_port(0.0f, 0.0f, static_cast<float>(back_buffer_desc.Width), static_cast<float>(back_buffer_desc.Height));
					this->context()->RSSetViewports(1, &view_port);
				}
				catch (int error)
				{
					return error;
				}
				return 0;
			}
		private:
			Microsoft::WRL::ComPtr<IDXGISwapChain> _swap_chain;
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _render_target_view;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depth_stencil_view;
		};


		class material_class : public base_context_class
		{
		public:
			material_class(const context_class& context) noexcept : base_context_class(context)
			{
			}
		public:
			int create_shader(D3D11_SHADER_VERSION_TYPE type, Microsoft::WRL::ComPtr<ID3DBlob> code) noexcept
			{
				try
				{
					failed_code(code == nullptr ? E_INVALIDARG : 0);

					const auto data = code->GetBufferPointer();
					const auto size = code->GetBufferSize();

					switch (type)
					{
						case D3D11_SHVER_PIXEL_SHADER:
						{
							failed_code(this->device()->CreatePixelShader(data, size, nullptr, this->_pixel_shader.ReleaseAndGetAddressOf()));
							break;
						}

						case D3D11_SHVER_VERTEX_SHADER:
						{
							Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
							failed_code(D3DReflect(data, size, IID_PPV_ARGS(reflection.GetAddressOf())));

							D3D11_SHADER_DESC shader_desc{};
							failed_code(reflection->GetDesc(&shader_desc));

							D3D11_INPUT_ELEMENT_DESC shader_input_element[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT]{};
							const UINT shader_input_element_count = min(ARRAYSIZE(shader_input_element), shader_desc.InputParameters);
							for (UINT i = 0; i != shader_input_element_count; i++)
							{
								D3D11_SIGNATURE_PARAMETER_DESC signature{};
								failed_code(reflection->GetInputParameterDesc(i, &signature));

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

							failed_code(this->device()->CreateInputLayout(shader_input_element, shader_input_element_count, data, size, this->_input_layout.ReleaseAndGetAddressOf()));
							failed_code(this->device()->CreateVertexShader(data, size, nullptr, this->_vertex_shader.ReleaseAndGetAddressOf()));
							break;
						}

						default:
						{
							failed_code(E_INVALIDARG);
						}
					}
				}
				catch (int error)
				{
					return error;
				}
				return 0;
			}

			int create_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, UINT size) noexcept
			{
				try
				{
					const CD3D11_BUFFER_DESC buffer_desc(size, D3D11_BIND_CONSTANT_BUFFER);

					Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
					failed_code(this->device()->CreateBuffer(&buffer_desc, nullptr, buffer.GetAddressOf()));
					this->create_shader_constant_buffer(type, index, buffer);
				}
				catch (int error)
				{
					return error;
				}
				return 0;
			}

			int create_texture(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data, UINT width, UINT height) noexcept
			{
				try
				{
					const CD3D11_TEXTURE2D_DESC texture_desc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

					const D3D11_SUBRESOURCE_DATA texture_data{
						.pSysMem = data,
						.SysMemPitch = width * sizeof(UINT),
					};

					Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
					failed_code(this->device()->CreateTexture2D(&texture_desc, &texture_data, texture.GetAddressOf()));
					this->create_shader_resource_view(type, index, texture);
				}
				catch (int error)
				{
					return error;
				}
				return 0;
			}

			int create_structured_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data, UINT count, UINT size) noexcept
			{
				try
				{
					const CD3D11_BUFFER_DESC buffer_desc(count * size, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, size);

					const D3D11_SUBRESOURCE_DATA buffer_data{
						.pSysMem = data,
						.SysMemPitch = size,
					};

					Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
					failed_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, buffer.GetAddressOf()));
					this->create_shader_resource_view(type, index, buffer);
				}
				catch (int error)
				{
					return error;
				}
				return 0;
			}
		public:
			void update_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, LPCVOID data) const noexcept
			{
				switch (type)
				{
					case D3D11_SHVER_PIXEL_SHADER:
					{
						if (index < ARRAYSIZE(this->_pixel_shader_constant_buffer))
						{
							this->context()->UpdateSubresource(this->_pixel_shader_constant_buffer[index].Get(), 0, nullptr, data, 0, 0);
						}
						break;
					}

					case D3D11_SHVER_VERTEX_SHADER:
					{
						if (index < ARRAYSIZE(this->_vertex_shader_constant_buffer))
						{
							this->context()->UpdateSubresource(this->_vertex_shader_constant_buffer[index].Get(), 0, nullptr, data, 0, 0);
						}
						break;
					}
				}
			}

			void use(D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) const noexcept
			{
				this->context()->IASetPrimitiveTopology(topology);

				this->context()->IASetInputLayout(this->_input_layout.Get());

				this->context()->VSSetShader(this->_vertex_shader.Get(), nullptr, 0);
				this->context()->VSSetConstantBuffers(0, ARRAYSIZE(this->_vertex_shader_constant_buffer), this->_vertex_shader_constant_buffer->GetAddressOf());
				this->context()->VSSetShaderResources(0, ARRAYSIZE(this->_vertex_shader_resource_view), this->_vertex_shader_resource_view->GetAddressOf());

				this->context()->PSSetShader(this->_pixel_shader.Get(), nullptr, 0);
				this->context()->PSSetConstantBuffers(0, ARRAYSIZE(this->_pixel_shader_constant_buffer), this->_pixel_shader_constant_buffer->GetAddressOf());
				this->context()->PSSetShaderResources(0, ARRAYSIZE(this->_pixel_shader_resource_view), this->_pixel_shader_resource_view->GetAddressOf());
			}
		private:
			void create_shader_constant_buffer(D3D11_SHADER_VERSION_TYPE type, UINT index, const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer)
			{
				switch (type)
				{
					case D3D11_SHVER_PIXEL_SHADER:
					{
						if (index < ARRAYSIZE(this->_pixel_shader_constant_buffer))
						{
							this->_pixel_shader_constant_buffer[index] = buffer;
							return;
						}
						break;
					}

					case D3D11_SHVER_VERTEX_SHADER:
					{
						if (index < ARRAYSIZE(this->_vertex_shader_constant_buffer))
						{
							this->_vertex_shader_constant_buffer[index] = buffer;
							return;
						}
						break;
					}
				}
				failed_code(E_INVALIDARG);
			}

			void create_shader_resource_view(D3D11_SHADER_VERSION_TYPE type, UINT index, const Microsoft::WRL::ComPtr<ID3D11Resource>& resource)
			{
				switch (type)
				{
					case D3D11_SHVER_PIXEL_SHADER:
					{
						if (index < ARRAYSIZE(this->_pixel_shader_resource_view))
						{
							failed_code(this->device()->CreateShaderResourceView(resource.Get(), nullptr, this->_pixel_shader_resource_view[index].ReleaseAndGetAddressOf()));
							return;
						}
						break;
					}

					case D3D11_SHVER_VERTEX_SHADER:
					{
						if (index < ARRAYSIZE(this->_vertex_shader_resource_view))
						{
							failed_code(this->device()->CreateShaderResourceView(resource.Get(), nullptr, this->_vertex_shader_resource_view[index].ReleaseAndGetAddressOf()));
							return;
						}
						break;
					}
				}
				failed_code(E_INVALIDARG);
			}
		private:
			Microsoft::WRL::ComPtr<ID3D11InputLayout> _input_layout;

			Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertex_shader;
			Microsoft::WRL::ComPtr<ID3D11Buffer> _vertex_shader_constant_buffer[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _vertex_shader_resource_view[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

			Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixel_shader;
			Microsoft::WRL::ComPtr<ID3D11Buffer> _pixel_shader_constant_buffer[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _pixel_shader_resource_view[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
		};


		class mesh_class : public base_context_class
		{
		public:
			mesh_class(const context_class& context, LPCVOID vertex_data, UINT vertex_data_size, LPCVOID index_data, UINT index_data_size) : base_context_class(context)
			{
				D3D11_BUFFER_DESC buffer_desc{};
				D3D11_SUBRESOURCE_DATA buffer_data{};

				buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;

				buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				buffer_desc.ByteWidth = vertex_data_size;
				buffer_data.pSysMem = vertex_data;
				failed_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, this->_vertex_buffer.GetAddressOf()));

				buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				buffer_desc.ByteWidth = index_data_size;
				buffer_data.pSysMem = index_data;
				failed_code(this->device()->CreateBuffer(&buffer_desc, &buffer_data, this->_index_buffer.GetAddressOf()));
			}
		public:
			void draw(UINT instance_count = 1) const noexcept
			{
				UINT strides[1]{ sizeof(float) * 8 }, offsets[1]{};
				this->context()->IASetVertexBuffers(0, 1, this->_vertex_buffer.GetAddressOf(), strides, offsets);
				this->context()->IASetIndexBuffer(this->_index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);

				D3D11_BUFFER_DESC buffer_desc{};
				this->_index_buffer->GetDesc(&buffer_desc);
				this->context()->DrawIndexedInstanced(buffer_desc.ByteWidth / sizeof(UINT), instance_count, 0, 0, 0);
			}
		private:
			Microsoft::WRL::ComPtr<ID3D11Buffer> _vertex_buffer, _index_buffer;
		};
	}
}
