// TestClientDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//



#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <Windows.h>
#include <cstdio>

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 256

SOCKET client;
sockaddr_in sai_client;

struct mPoint
{
	int x;
	int y;
};

// Message
struct uMsg
{
	int type;
	char name[64];
	char text[1024]; // text msg
	mPoint *m_point;
};


void recvMessage()
{
	while (1) {
		uMsg msg;
		int ret_recv = recv(client, (char*)&msg, sizeof(msg), 0);
		if (ret_recv <= 0) {
			std::cout << "recv failed: "<< GetLastError() << std::endl;
			break;
		}

		std::cout << msg.name << ": " << msg.text << std::endl;
	}
}

int main()
{
	std::cout << "****************\n*    CLIENT    *\n****************\n\n";

	// Initialize Winsock
	WSADATA wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) 
	{
		std::cout << "WSAStartup Failed with error: " << iResult << std::endl;
		return 1;
	}

	SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) 
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &addrServer.sin_addr.s_addr);
	addrServer.sin_port = htons(5555);
	memset(&(addrServer.sin_zero), '\0', 8);

	std::cout << "Connecting..." << std::endl;
	iResult = connect(ConnectSocket, (SOCKADDR *)&addrServer, sizeof(addrServer));
	if (iResult == SOCKET_ERROR) 
	{
		closesocket(ConnectSocket);
		std::cout << "Unable to connect to server: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// input username
	uMsg msg;
	msg.type = 1;
	std::string name;
	getline(std::cin, name);
	strncpy_s(msg.name, sizeof(msg.name), name.c_str(), 64);
	strncpy_s(msg.text, sizeof(msg.text), "", 512);
	int error_send;

	// Send file name
	iResult = send(ConnectSocket, (char*)&msg, sizeof(msg), 0);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// recv server info
	HANDLE h_recvMes = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)recvMessage, 0, 0, 0);
	if (!h_recvMes) {
		std::cout << "Create thread failed : " << GetLastError() << std::endl;
	}

	// send msg
	while (1)
	{
		std::string content;
		getline(std::cin, content);

		if (content == "quit") {
			msg.type = 2;
			send(client, (char*)&msg, sizeof(msg), 0);
			error_send = GetLastError();
			if (error_send != 0) {
				std::cout << "send failed:" << error_send << std::endl;
			}
			closesocket(client);
			WSACleanup();
			return 0;
		}
		
		msg.type = 3;
		strncpy_s(msg.text, sizeof(msg.text), content.c_str(), 512);
		send(client, (char*)&msg, sizeof(msg), 0);
		error_send = GetLastError();
		if (error_send != 0)
		{
			std::cout << "send failed: " << error_send << std::endl;
		}
	}

	getchar();
	return 0;
}


