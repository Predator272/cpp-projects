#include "main.hpp"

class camera
{
public:
	constexpr camera() noexcept : position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), angle(DirectX::XM_PIDIV2), aspect_ratio(1.0f)
	{
	}

	constexpr camera(const DirectX::XMFLOAT3A& position, const DirectX::XMFLOAT3A& rotation, float angle, float aspect_ratio) noexcept : position(position), rotation(rotation), angle(angle), aspect_ratio(aspect_ratio)
	{
	}
public:
	void get(DirectX::XMFLOAT4X4A& output) const noexcept
	{
		const auto vector = DirectX::XMLoadFloat3A(&this->position);
		const auto matrix = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&this->rotation));
		const auto target = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), matrix);
		const auto up = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), matrix);
		DirectX::XMStoreFloat4x4A(&output, DirectX::XMMatrixMultiply(DirectX::XMMatrixLookAtLH(vector, DirectX::XMVectorAdd(vector, target), up), DirectX::XMMatrixPerspectiveFovLH(this->angle, this->aspect_ratio, 0.01f, 100000.0f)));
	}

	void transform(DirectX::XMFLOAT4X4A& output, const DirectX::XMFLOAT3A& position = DirectX::XMFLOAT3A(0.0f, 0.0f, 0.0f), const DirectX::XMFLOAT3A& rotation = DirectX::XMFLOAT3A(0.0f, 0.0f, 0.0f), const DirectX::XMFLOAT3A& scale = DirectX::XMFLOAT3A(1.0f, 1.0f, 1.0f)) const noexcept
	{
		const auto matrix_position = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3A(&position));
		const auto matrix_rotation = DirectX::XMMatrixRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rotation));
		const auto matrix_scale = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3A(&scale));
		DirectX::XMStoreFloat4x4A(&output, DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(matrix_rotation, matrix_position), matrix_scale));
	}
public:
	DirectX::XMFLOAT3A position, rotation;
	float angle, aspect_ratio;
};

constexpr float vertices[]{
	-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-1.0f, -1.0f, +1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	-1.0f, +1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	+1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	+1.0f, -1.0f, +1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	+1.0f, +1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
	+1.0f, +1.0f, +1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
};

constexpr UINT indexes[]{
	0, 2, 4, 6, 4, 2,
	4, 6, 5, 7, 5, 6,
	5, 7, 1, 3, 1, 7,
	1, 3, 0, 2, 0, 3,
	2, 3, 6, 7, 6, 3,
	1, 0, 5, 4, 5, 0,
};

std::wstring image_filename;

class main_renderer : public engine::d3d11renderer
{
private:
	std::unique_ptr<engine::d3d11material> material;
	std::unique_ptr<engine::d3d11mesh> mesh;

	struct
	{
		DirectX::XMFLOAT4X4A view_projection;
		DirectX::XMFLOAT4X4A world;
	} constant_buffer{};

	camera camera_object;

	int frameCount = 0;
	float lastTime = 0.0f;
public:
	void create(const engine::event& event) override
	{
		material = std::make_unique<engine::d3d11material>(*this);
		material->create_shader(D3D11_SHVER_VERTEX_SHADER, engine::compile_shader("vs_5_0", 0x0001));
		material->create_shader(D3D11_SHVER_PIXEL_SHADER, engine::compile_shader("ps_5_0", 0x0002));
		material->create_constant_buffer(D3D11_SHVER_VERTEX_SHADER, 0, sizeof(constant_buffer));

		mesh = std::make_unique<engine::d3d11mesh>(*this, vertices, sizeof(vertices), indexes, sizeof(indexes));
	}

	UINT update(const engine::event& event, const SIZE& size) override
	{
		const float time = (float)engine::clock();

		this->clear(0.0f, 0.0f, 0.0f, 1.0f);

		if (!image_filename.empty())
		{
			try
			{
				UINT width, height;
				auto image = engine::load_image_from_file(image_filename.c_str(), width, height);
				material->create_texture(D3D11_SHVER_PIXEL_SHADER, 0, image->GetBufferPointer(), width, height);
			}
			catch (...)
			{
			}
			image_filename.clear();
		}

		material->use();
		camera_object.aspect_ratio = (float)(size.cx) / (float)(size.cy);
		camera_object.get(constant_buffer.view_projection);
		camera_object.transform(constant_buffer.world, { 0.0f, 0.0f, 5.0f }, { time, time, time });
		material->update_constant_buffer(D3D11_SHVER_VERTEX_SHADER, 0, &constant_buffer);

		mesh->draw();

		frameCount++;
		if (time - lastTime >= 1.0f)
		{
			std::array<WCHAR, 0x40> string;
			StringCchPrintfW(string.data(), string.size(), L"FPS: %0.1f", frameCount / (time - lastTime));
			SetWindowTextW(event.window(), string.data());
			frameCount = 0;
			lastTime = time;
		}

		return 0;
	}
};

main_renderer renderer;

bool default_handler(const engine::event& event)
{
	switch (event.type())
	{
		case WM_CREATE:
		{
			DragAcceptFiles(event.window(), true);
			SetWindowTextW(event.window(), L"Simple");
			ShowWindow(event.window(), SW_SHOW);
			return true;
		}

		case WM_DROPFILES:
		{
			image_filename.resize(MAX_PATH);
			DragQueryFileW((HDROP)event.wparam(), 0, image_filename.data(), image_filename.size());
			DragFinish((HDROP)event.wparam());
			return true;
		}
	}
	return false;
}

bool engine::main_handler(const engine::event& event)
{
	bool result = false;
	result |= renderer.handler(event);
	return default_handler(event) || result;
}
