#include "main.hpp"


class timer_class
{
public:
	timer_class() noexcept : _current((float)clock()), _delta(0), _elapsed(0)
	{
	}
public:
	timer_class& update() noexcept
	{
		const auto temp = this->_current;
		this->_current = (float)clock();
		this->_elapsed += this->_delta = this->_current - temp;
		return *this;
	}

	timer_class& reset() noexcept
	{
		this->_elapsed = 0;
		return *this;
	}
public:
	float current() const noexcept
	{
		return this->_current;
	}

	float delta() const noexcept
	{
		return this->_delta;
	}

	float elapsed() noexcept
	{
		return this->_elapsed;
	}
private:
	float _current, _delta, _elapsed;
};


class snake_class
{
public:
	enum direction
	{
		direction_up,
		direction_down,
		direction_left,
		direction_right,
	};

	using point = std::pair<int, int>;
	using point_list = std::list<point>;

	static constexpr auto GRID_SIZE_X = 24, GRID_SIZE_Y = 16, GRID_TOTAL_SIZE = GRID_SIZE_X * GRID_SIZE_Y;
	static constexpr auto START_SPEED = 2.0f;
	static constexpr auto START_LENGTH = 5;
	static constexpr auto START_DIRECTION = direction::direction_right;
	static constexpr auto START_POSITION = point(GRID_SIZE_X / 2, GRID_SIZE_Y / 2);
public:
	bool handle()
	{
		if (this->_snake.empty())
		{
			return this->_move();
		}
		else
		{
			if (this->_pause)
			{
				this->_timer.reset();
			}
			else
			{
				if (this->_timer.update().elapsed() >= (1.0f / START_SPEED))
				{
					this->_timer.reset();
					return this->_move();
				}
			}
		}
		return false;
	}

	size_t get_score() const noexcept
	{
		return std::clamp<size_t>(this->_snake.size(), START_LENGTH, GRID_TOTAL_SIZE) - START_LENGTH;
	}

	const point_list& get_snake() const noexcept
	{
		return this->_snake;
	}

	const point& get_food() const noexcept
	{
		return this->_food;
	}

	void set_direction(direction value) noexcept
	{
		if (this->_pause == false && this->_new_direction == this->_direction)
		{
			if ((value == direction::direction_up && this->_new_direction != direction::direction_down) ||
				(value == direction::direction_down && this->_new_direction != direction::direction_up) ||
				(value == direction::direction_left && this->_new_direction != direction::direction_right) ||
				(value == direction::direction_right && this->_new_direction != direction::direction_left))
			{
				this->_new_direction = value;
			}
		}
	}

	void set_pause(bool value) noexcept
	{
		this->_pause = value;
	}

	bool get_pause() const noexcept
	{
		return this->_pause;
	}
private:
	bool _set_food()
	{
		if (this->_snake.size() < GRID_TOTAL_SIZE)
		{
			std::random_device device;
			std::mt19937 gen(device());
			std::uniform_int_distribution eat_x(0, GRID_SIZE_X - 1), eat_y(0, GRID_SIZE_Y - 1);

			do
			{
				this->_food = point(eat_x(gen), eat_y(gen));
			}
			while (_contain_element(this->_food));

			return false;
		}
		this->_snake.clear();
		return true;
	}

	point _get_vector() noexcept
	{
		if (this->_direction != this->_new_direction)
		{
			this->_direction = this->_new_direction;
		}

		switch (this->_direction)
		{
			case direction::direction_up: return point(0, -1);
			case direction::direction_down: return point(0, +1);
			case direction::direction_left: return point(-1, 0);
			case direction::direction_right: return point(+1, 0);
		}
		return point(0, 0);
	}

	bool _contain_element(const point& element)
	{
		return std::find(this->_snake.begin(), this->_snake.end(), element) != this->_snake.end();
	}

	bool _move()
	{
		if (this->_snake.empty())
		{
			this->_pause = false;
			this->_direction = this->_new_direction = START_DIRECTION;
			this->_timer.reset();
			this->_snake.push_back(START_POSITION);
			while (this->_snake.size() != START_LENGTH)
			{
				if (this->_move())
				{
					return true;
				}
			}
			return this->_set_food();
		}
		else
		{
			const auto vector = this->_get_vector();
			const auto last = this->_snake.back();
			auto new_element = point(last.first + vector.first, last.second + vector.second);

			if (new_element.first < 0) new_element.first = GRID_SIZE_X - 1;
			if (new_element.second < 0) new_element.second = GRID_SIZE_Y - 1;
			if (new_element.first >= GRID_SIZE_X) new_element.first = 0;
			if (new_element.second >= GRID_SIZE_Y) new_element.second = 0;

			if (this->_snake.size() >= START_LENGTH && new_element != this->_food)
			{
				this->_snake.pop_front();
			}

			if (new_element.first < 0 || new_element.second < 0 || new_element.first >= GRID_SIZE_X || new_element.second >= GRID_SIZE_Y || this->_contain_element(new_element))
			{
				this->_snake.clear();
				return true;
			}

			this->_snake.push_back(new_element);
			if (new_element == this->_food)
			{
				return this->_set_food();
			}
		}
		return false;
	}
private:
	bool _pause;
	timer_class _timer;
	direction _direction, _new_direction;
	point_list _snake;
	point _food;
};


class application : public engine::application
{
public:
	static constexpr auto GAME_AREA_SIZE_X = 640, GAME_AREA_SIZE_Y = 480;
private:
	static constexpr auto GRID_ELEMENT_SIZE = 24, GRID_ELEMENT_SPACE = 2, GRID_SIZE_X = snake_class::GRID_SIZE_X, GRID_SIZE_Y = snake_class::GRID_SIZE_Y;
	static constexpr auto GRID_BORDER_SIZE = GRID_ELEMENT_SPACE * 2, GRID_ELEMENT_TOTAL_SIZE = GRID_ELEMENT_SIZE + GRID_ELEMENT_SPACE;
	static constexpr auto GRID_AREA_SIZE_X = GRID_ELEMENT_SPACE * 3 + ((GRID_ELEMENT_SIZE + GRID_ELEMENT_SPACE) * GRID_SIZE_X);
	static constexpr auto GRID_AREA_SIZE_Y = GRID_ELEMENT_SPACE * 3 + ((GRID_ELEMENT_SIZE + GRID_ELEMENT_SPACE) * GRID_SIZE_Y);
private:
	Microsoft::WRL::ComPtr<ID2D1Factory> _factory;
	Microsoft::WRL::ComPtr<IDWriteFactory> _write_factory;
	Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> _render_target;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> _text_brush;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> _text_format;
private:
	snake_class _snake;
private:
	void draw_rect(const D2D1_RECT_F& rect, const Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush, float stroke = 0.0f) const noexcept
	{
		const auto new_rect = D2D1::RectF(rect.left, rect.top, rect.left + rect.right, rect.top + rect.bottom);
		if (stroke > 0.0f)
		{
			const auto border = stroke / 2.0f;
			this->_render_target->DrawRectangle(D2D1::RectF(new_rect.left + border, new_rect.top + border, new_rect.right - border, new_rect.bottom - border), brush.Get(), stroke);
		}
		else
		{
			this->_render_target->FillRectangle(new_rect, brush.Get());
		}
	}

	void draw_text(const std::wstring& string, const D2D1_RECT_F& rect, const Microsoft::WRL::ComPtr<IDWriteTextFormat>& format, const Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush) const noexcept
	{
		const auto new_rect = D2D1::RectF(rect.left, rect.top, rect.left + rect.right, rect.top + rect.bottom);
		this->_render_target->DrawTextW(string.data(), (UINT32)string.size(), format.Get(), new_rect, brush.Get());
	}

	bool handler(HWND window, UINT message, WPARAM wparam, LPARAM lparam) override
	{
		switch (message)
		{
			case WM_CREATE:
			{
				engine::throw_code(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, this->_factory.GetAddressOf()));
				engine::throw_code(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(this->_write_factory.GetAddressOf())));
				engine::throw_code(this->_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(window), this->_render_target.GetAddressOf()));

				engine::throw_code(this->_render_target->CreateSolidColorBrush(D2D1::ColorF(0x003000), this->_text_brush.GetAddressOf()));
				engine::throw_code(this->_write_factory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"", this->_text_format.GetAddressOf()));
				engine::throw_code(this->_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
				engine::throw_code(this->_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
				return true;
			}

			case WM_SIZE:
			{
				engine::throw_code(this->_render_target->Resize(D2D1::SizeU(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam))));
				return true;
			}

			case WM_PAINT:
			{
				this->_render_target->BeginDraw();
				this->_render_target->Clear(D2D1::ColorF(0x00AF00));

				this->_snake.handle();

				const auto _get_window_size = this->_render_target->GetSize();
				const auto game_area = D2D1::RectF((_get_window_size.width - GAME_AREA_SIZE_X) / 2, (_get_window_size.height - GAME_AREA_SIZE_Y) / 2, GAME_AREA_SIZE_X, GAME_AREA_SIZE_Y);
				const auto text_area = D2D1::RectF(game_area.left + 5, game_area.top + 5, game_area.right - 10, 48);
				const auto text_score_area = D2D1::RectF(text_area.left, text_area.top, text_area.right / 2, text_area.bottom);
				const auto text_pause_area = D2D1::RectF(text_area.left + text_score_area.right, text_area.top, text_score_area.right, text_area.bottom);
				const auto grid_area = D2D1::RectF(game_area.left + (game_area.right - GRID_AREA_SIZE_X) / 2, text_area.top + text_area.bottom, GRID_AREA_SIZE_X, GRID_AREA_SIZE_Y);
				const auto grid_render_area = D2D1::RectF(grid_area.left + GRID_BORDER_SIZE, grid_area.top + GRID_BORDER_SIZE, grid_area.right - GRID_BORDER_SIZE * 2, grid_area.bottom - GRID_BORDER_SIZE * 2);

				this->draw_rect(D2D1::RectF(grid_render_area.left + (this->_snake.get_food().first * GRID_ELEMENT_TOTAL_SIZE), grid_render_area.top + (this->_snake.get_food().second * GRID_ELEMENT_TOTAL_SIZE), GRID_ELEMENT_SIZE, GRID_ELEMENT_SIZE), this->_text_brush, GRID_ELEMENT_SPACE);
				for (const auto& element : this->_snake.get_snake())
				{
					this->draw_rect(D2D1::RectF(grid_render_area.left + (element.first * GRID_ELEMENT_TOTAL_SIZE), grid_render_area.top + (element.second * GRID_ELEMENT_TOTAL_SIZE), GRID_ELEMENT_SIZE, GRID_ELEMENT_SIZE), this->_text_brush);
				}

				this->draw_rect(grid_area, this->_text_brush, GRID_ELEMENT_SPACE);
				this->draw_text(std::format(L"SCORE: {}", this->_snake.get_score()), text_score_area, this->_text_format, this->_text_brush);
				if (this->_snake.get_pause())
				{
					this->draw_text(L"PAUSE", text_pause_area, this->_text_format, this->_text_brush);
				}

				engine::throw_code(this->_render_target->EndDraw());
				return true;
			}

			case WM_KEYDOWN:
			{
				switch (wparam)
				{
					case 0x57:
					case VK_UP:
					{
						this->_snake.set_direction(snake_class::direction::direction_up);
						break;
					}

					case 0x53:
					case VK_DOWN:
					{
						this->_snake.set_direction(snake_class::direction::direction_down);
						break;
					}

					case 0x41:
					case VK_LEFT:
					{
						this->_snake.set_direction(snake_class::direction::direction_left);
						break;
					}

					case 0x44:
					case VK_RIGHT:
					{
						this->_snake.set_direction(snake_class::direction::direction_right);
						break;
					}

					case 0x50:
					case VK_ESCAPE:
					case VK_SPACE:
					{
						this->_snake.set_pause(!this->_snake.get_pause());
						break;
					}
				}
				return true;
			}

			case WM_KILLFOCUS:
			{
				this->_snake.set_pause(true);
				return true;
			}
		}
		return false;
	}
};


int entry()
{
	const int width = application::GAME_AREA_SIZE_X, height = application::GAME_AREA_SIZE_Y;
	const int left = (GetSystemMetrics(SM_CXFULLSCREEN) - width) / 2, top = (GetSystemMetrics(SM_CYFULLSCREEN) - height) / 2;

	return application().run(L"Snake", WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, { left, top, width, height });
}
