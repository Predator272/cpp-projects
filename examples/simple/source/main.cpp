#include "main.hpp"

class d3d12handler : public d3d12context
{
protected:
	virtual void on_create(HWND hwnd) {}
	virtual void on_update(HWND hwnd) {}
	virtual void on_destroy() {}
public:
	d3d12handler(const engine::application::window& window) noexcept : d3d12context(window.hwnd(), window.size()), _critical_section{}, _thread(nullptr), _hwnd(nullptr)
	{
		const auto hwnd = window.hwnd();
		InitializeCriticalSection(&this->_critical_section);
		if (hwnd && IsWindow(hwnd))
		{
			this->_hwnd = hwnd;
			this->_thread = CreateThread(nullptr, 0, _handler, this, 0, nullptr);
		}
	}

	~d3d12handler() noexcept
	{
		if (WaitForSingleObject(this->_thread, INFINITE) == WAIT_OBJECT_0)
		{
			CloseHandle(this->_thread);
			DeleteCriticalSection(&this->_critical_section);
		}
	}
private:
	static DWORD WINAPI _handler(LPVOID param) noexcept
	{
		try
		{
			auto _this = reinterpret_cast<d3d12handler*>(param);
			EnterCriticalSection(&_this->_critical_section);
			_this->on_create(_this->_hwnd);
			LeaveCriticalSection(&_this->_critical_section);
			while (_this->valid_hwnd())
			{
				EnterCriticalSection(&_this->_critical_section);
				_this->on_update(_this->_hwnd);
				LeaveCriticalSection(&_this->_critical_section);
			}
			EnterCriticalSection(&_this->_critical_section);
			_this->on_destroy();
			LeaveCriticalSection(&_this->_critical_section);
			return 0;
		}
		catch (...)
		{
			return E_FAIL;
		}
	}

	bool valid_hwnd() noexcept
	{
		EnterCriticalSection(&this->_critical_section);
		const auto result = this->_hwnd && IsWindow(this->_hwnd);
		LeaveCriticalSection(&this->_critical_section);
		return result;
	}
private:
	CRITICAL_SECTION _critical_section;
	HANDLE _thread = nullptr;
	HWND _hwnd = nullptr;
};

class d3d12renderer : public d3d12handler
{
public:
	d3d12renderer(const engine::application::window& window) : d3d12handler(window)
	{
	}
private:
	void on_create(HWND hwnd) override
	{
	}

	void on_update(HWND hwnd) override
	{
		const auto current_window_size = _get_window_size(hwnd);
		if (memcmp(&this->_size, &current_window_size, sizeof(SIZE)))
		{
			memcpy(&this->_size, &current_window_size, sizeof(SIZE));
			if (this->_size.cx > 0 && this->_size.cy > 0)
			{
				this->resize(this->_size);
			}
		}
		this->render(0.0f, 1.0f, 0.0f, [&](com_ptr_type<ID3D12GraphicsCommandList>&) {

			const auto string = std::format(L"{}", engine::clock());
			SetWindowTextW(hwnd, string.c_str());
		});
	}
private:
	static SIZE _get_window_size(HWND hwnd) noexcept
	{
		RECT rect;
		if (GetClientRect(hwnd, &rect))
		{
			return { rect.right - rect.left, rect.bottom - rect.top };
		}
		return {};
	}
private:
	SIZE _size = {};
};

class main_application : public engine::application
{
private:
	std::optional<LRESULT> handler(const engine::application::event& event) override
	{
		switch (event.type())
		{
			case WM_CREATE:
			{
				//this->_renderer = std::make_unique<d3d12renderer>(event.window());
				break;
			}
		}
		return std::nullopt;
	}
private:
	std::unique_ptr<d3d12renderer> _renderer;
};

int engine::main()
{
	return main_application().run({});
}
