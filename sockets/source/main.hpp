#pragma once

#include <print>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <span>
#include <cmath>
#include <random>
#include <numeric>
#include <ranges>
#include <functional>
#include <execution>
#include <valarray>
#include <chrono>

#include <windows.h>
#pragma comment(lib, "ws2_32")

class AsyncSocketAPI
{
public:
	// Управление
	virtual void start(int port) = 0;
	virtual void stop() = 0;

	// События
	virtual void onReceive(const std::string& data, SOCKET client) = 0;
	virtual void onSend(size_t bytes, SOCKET client) = 0;
	virtual void onError(const std::string& msg) = 0;
};

void safe_throw_code(const int code)
{
	if (std::uncaught_exceptions() == 0)
	{
		throw code;
	}
}

void check_code(const int code)
{
	if (code)
	{
		safe_throw_code(code);
	}
}

class WindowsSocket
{
public:
	WindowsSocket()
	{
		WSADATA wsa_data;
		check_code(WSAStartup(MAKEWORD(2, 2), &wsa_data));
	}

	~WindowsSocket()
	{
		check_code(WSACleanup());
	}
};

class AsyncTcpServer : public AsyncSocketAPI, public WindowsSocket
{
private:
	std::atomic<bool> running;
	SOCKET listenSocket;
	std::thread worker;
public:
	AsyncTcpServer() : running(false), listenSocket(INVALID_SOCKET)
	{
	}

	void start(int port) override
	{
		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET)
		{
			onError("Ошибка socket()");
			return;
		}

		sockaddr_in service{};
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = INADDR_ANY;
		service.sin_port = htons(port);

		if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
		{
			onError("Ошибка bind()");
			return;
		}

		if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			onError("Ошибка listen()");
			return;
		}

		running = true;
		CreateThread(nullptr, 0, )


		worker = std::thread([this]() { eventLoop(); });
	}

	void event()
	{

	}

	void stop() override
	{
		running = false;
		if (worker.joinable()) worker.join();
		if (listenSocket != INVALID_SOCKET) closesocket(listenSocket);
	}

protected:
	void eventLoop()
	{
		fd_set readfds;
		while (running)
		{
			FD_ZERO(&readfds);
			FD_SET(listenSocket, &readfds);

			timeval tv{};
			tv.tv_sec = 1; // таймаут
			tv.tv_usec = 0;

			int res = select(0, &readfds, nullptr, nullptr, &tv);
			if (res > 0 && FD_ISSET(listenSocket, &readfds))
			{
				SOCKET client = accept(listenSocket, nullptr, nullptr);
				if (client != INVALID_SOCKET)
				{
					std::thread([this, client]() {
						char buffer[1024];
						while (true)
						{
							int n = recv(client, buffer, sizeof(buffer), 0);
							if (n > 0)
							{
								onReceive(std::string(buffer, n), client);
							}
							else if (n == 0)
							{
								break; // клиент закрыл соединение
							}
							else
							{
								onError("Ошибка recv()");
								break;
							}
						}
						closesocket(client);
					}).detach();
				}
			}
		}
	}

	// Виртуальные методы
	void onReceive(const std::string& data, SOCKET client) override
	{
		std::cout << "Получено: " << data << std::endl;
		std::string reply = "Echo: " + data;
		int sent = send(client, reply.c_str(), (int)reply.size(), 0);
		if (sent > 0) onSend(sent, client);
	}

	void onSend(size_t bytes, SOCKET) override
	{
		std::cout << "Отправлено " << bytes << " байт" << std::endl;
	}

	void onError(const std::string& msg) override
	{
		std::cerr << "Ошибка: " << msg << std::endl;
	}
};
