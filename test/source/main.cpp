#include "main.hpp"

class application : public engine::application, public engine::raw_input, public engine::d3d11renderer
{
private:
	static constexpr float vertices[]{
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, //0
		-1.0f, -1.0f, +1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, //1
		-1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //2
		-1.0f, +1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, //3
		+1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, //4
		+1.0f, -1.0f, +1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, //5
		+1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, //6
		+1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, //7
	};

	static constexpr UINT indexes[]{
		0, 2, 4, 6, 4, 2,
		4, 6, 5, 7, 5, 6,
		5, 7, 1, 3, 1, 7,
		1, 3, 0, 2, 0, 3,
		2, 3, 6, 7, 6, 3,
		1, 0, 5, 4, 5, 0,
	};

	static constexpr UINT texture_buffer_source[]{
		0xFF0000FF, 0xFF00FF00,
		0xFFFF0000, 0xFF00FFFF,
	};
private:
	std::unique_ptr<engine::d3d11material> material;
	std::unique_ptr<engine::d3d11mesh> mesh;

	std::vector<DirectX::XMMATRIX> transformation;

	class constant_buffer_class
	{
	public:
		DirectX::XMFLOAT4X4A view_projection;
		DirectX::XMFLOAT4X4A world;
	} constant_buffer{};

	camera_class camera;
	timer_class timer;
	float elapsed_time = 0.0f;
	size_t frame_counter = 0;

	bool keys[0x100]{};

	struct
	{
		BYTE key_forward = 'W';
		BYTE key_backward = 'S';
		BYTE key_left = 'A';
		BYTE key_right = 'D';
		BYTE key_up = 'R';
		BYTE key_down = 'F';
		BYTE key_run = VK_LSHIFT;
	} settings;
private:
	bool handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam) override
	{
		auto result = this->handle_default(window, message, wparam, lparam);
		result |= engine::d3d11renderer::handle(window, message, wparam, lparam);
		result |= engine::raw_input::handle(window, message, wparam, lparam);
		return result;
	}

	bool handle_default(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
			case WM_SIZE:
			{
				const float width = GET_X_LPARAM(lparam), height = GET_Y_LPARAM(lparam);
				this->camera.aspect_ratio = width / height;
				return true;
			}
		}
		return false;
	}

	void handle_d3drenderer(HWND window, UINT width, UINT height) override
	{
		this->timer.update();

		if (!this->material)
		{
			this->material = std::make_unique<engine::d3d11material>(*this);
			this->material->create_shader(D3D11_SHVER_VERTEX_SHADER, engine::compile_shader("vs_5_0", IDR_VS));
			this->material->create_shader(D3D11_SHVER_PIXEL_SHADER, engine::compile_shader("ps_5_0", IDR_PS));
			this->material->create_constant_buffer(D3D11_SHVER_VERTEX_SHADER, 0, sizeof(this->constant_buffer));
			this->material->create_texture(D3D11_SHVER_PIXEL_SHADER, 0, texture_buffer_source, 2, 2);

			static constexpr size_t grid_size = 50;
			static constexpr float space = 10.0f;

			for (size_t x = 0; x != grid_size; x++)
			{
				for (size_t y = 0; y != grid_size; y++)
				{
					for (size_t z = 0; z != grid_size; z++)
					{
						this->transformation.push_back(DirectX::XMMatrixTranslation(x * space, y * space, z * space));
					}
				}
			}
			this->material->create_structured_buffer(D3D11_SHVER_VERTEX_SHADER, 0, this->transformation.data(), this->transformation.size(), sizeof(this->transformation[0]));
		}

		if (!this->mesh)
		{
			this->mesh = std::make_unique<engine::d3d11mesh>(*this, vertices, sizeof(vertices), indexes, sizeof(indexes));
		}

		DirectX::XMVECTOR direction = DirectX::XMVectorZero();
		const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(0.0f, this->camera.rotation.y, 0.0f);
		const DirectX::XMVECTOR forward = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
		const DirectX::XMVECTOR right = DirectX::XMVector3Transform(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
		const DirectX::XMVECTOR up = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);

		if (this->keys[this->settings.key_forward]) direction = DirectX::XMVectorAdd(direction, forward);
		if (this->keys[this->settings.key_backward]) direction = DirectX::XMVectorSubtract(direction, forward);
		if (this->keys[this->settings.key_right]) direction = DirectX::XMVectorAdd(direction, right);
		if (this->keys[this->settings.key_left]) direction = DirectX::XMVectorSubtract(direction, right);
		if (this->keys[this->settings.key_up]) direction = DirectX::XMVectorAdd(direction, up);
		if (this->keys[this->settings.key_down]) direction = DirectX::XMVectorSubtract(direction, up);

		direction = DirectX::XMVector3Normalize(direction);

		if (this->keys[this->settings.key_run]) direction = DirectX::XMVectorMultiply(direction, DirectX::XMVectorReplicate(100.0f));

		DirectX::XMStoreFloat3A(&this->camera.position, DirectX::XMVectorMultiplyAdd(direction, DirectX::XMVectorReplicate(this->timer.delta()), DirectX::XMLoadFloat3A(&this->camera.position)));
		this->constant_buffer.view_projection = this->camera.get();

		this->material->use();
		this->constant_buffer.world = this->camera.transform();
		this->material->update_constant_buffer(D3D11_SHVER_VERTEX_SHADER, 0, &this->constant_buffer);
		this->mesh->draw(this->transformation.size());

		this->frame_counter++;
		if ((this->elapsed_time += this->timer.delta()) >= 0.5f)
		{
			WCHAR buffer[0x100]{};
			StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"FPS: %0.1f; Elements: %llu", this->frame_counter / this->elapsed_time, this->transformation.size());
			SetWindowTextW(window, buffer);
			this->elapsed_time = 0;
			this->frame_counter = 0;
		}
	}

	void handle_raw_input(const RAWINPUT& input) override
	{
		switch (input.header.dwType)
		{
			case RIM_TYPEMOUSE:
			{
				this->camera.rotation.y += DirectX::XMConvertToRadians(static_cast<float>(input.data.mouse.lLastX) * 0.1f);
				this->camera.rotation.x += DirectX::XMConvertToRadians(static_cast<float>(input.data.mouse.lLastY) * 0.1f);
				break;
			}

			case RIM_TYPEKEYBOARD:
			{
				const auto key_code = static_cast<BYTE>(MapVirtualKeyW(input.data.keyboard.MakeCode, MAPVK_VSC_TO_VK_EX));
				this->keys[key_code] = (input.data.keyboard.Flags != RI_KEY_BREAK);
				break;
			}
		}
	}
};

int entry()
{
	const int width = 640, height = 480;
	const int left = (GetSystemMetrics(SM_CXFULLSCREEN) - width) / 2, top = (GetSystemMetrics(SM_CYFULLSCREEN) - height) / 2;

	return application().run(L"Game", WS_VISIBLE | WS_OVERLAPPEDWINDOW, { left, top, width, height });
}
