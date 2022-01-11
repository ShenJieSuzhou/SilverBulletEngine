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
	int x;
	int y;
	int z;
	//int length;
	//char content[4096];
	//const char *content;
	//char name[64];
	//char text[512]; // text msg
	//mPoint *m_point;
};

//typedef struct userClientNode
//{
//	int fd;
//
//	//struct event_base *evbase;
//
//	//struct bufferevent *buf_ev;
//
//	//struct evbuffer *output_buffer;
//
//	struct uMsg *clientInfo;
//
//	userClientNode *next;
//
//} *ucnode_t;


void recvMessage()
{
	while (1) {
		int type;
		size_t sz = recv(client, (char *)&type, sizeof(int), 0);
		if (sz <= 0)
		{	
			std::cout << "recv failed: " << GetLastError() << std::endl;
			break;
		}

		// 新用户上线
		if (type == 200)
		{
			int len = 0;
			recv(client, (char *)&len, sizeof(int), 0);
			char *buf = new char(len + 1);
			recv(client, buf, len, 0);
			buf[len] = '\0';
			printf("新用户 ip 地址: %s\n", buf);
		}
		else if (type == 201)
		{
			// 坐标
		}
	}

	/*char buf[1024];
	size_t sz;
	while (1) {
	sz = recv(client, buf, sizeof(buf), 0);
	if (sz <= 0) continue;
	buf[sz] = '\0';
	std::cout << buf << "\n";
	}
	std::cout << "Recv cp \n";*/

	//uMsg msg;
	//int ret_recv = recv(client, (char*)&msg, sizeof(msg), 0);
	//if (ret_recv <= 0) {
	//	std::cout << "recv failed: "<< GetLastError() << std::endl;
	//	break;
	//}

	//std::cout << msg.name << ": " << msg.text << std::endl;
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

	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET)
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
	iResult = connect(client, (SOCKADDR *)&addrServer, sizeof(addrServer));
	if (iResult == SOCKET_ERROR) 
	{
		closesocket(client);
		std::cout << "Unable to connect to server: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// Input username
	//uMsg msg;
	//msg.type = 1;
	//std::string name;
	//getline(std::cin, name);
	//strncpy_s(msg.name, sizeof(msg.name), name.c_str(), 64);
	//strncpy_s(msg.text, sizeof(msg.text), "", 512);
	//int error_send;

	// Send file name
	//iResult = send(client, (char*)&name, sizeof(name), 0);
	//if (iResult == SOCKET_ERROR) {
	//	std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
	//	WSACleanup();
	//	return 1;
	//}

	//// Recv server info
	HANDLE h_recvMes = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)recvMessage, 0, 0, 0);
	if (!h_recvMes) {
		std::cout << "Create thread failed : " << GetLastError() << std::endl;
		return 1;
	}

	int inc = 0;
	// Send msg
	while (1)
	{
		//std::string content;
		//getline(std::cin, content);
		//if (content == "q") {
		//	closesocket(client);
		//	WSACleanup();
		//	return 0;
		//}

		uMsg msg;
		msg.type = 201;
		msg.x = inc;
		msg.y = inc;
		msg.z = inc;
		//msg.type = 1;
		/*msg.length = content.size();
		strcpy_s(msg.content, content.c_str());*/
		//strcpy(msg.content, content.c_str());
		//msg.content = content.c_str();
		//msg.content = content.c_str();
		send(client, (char*)&msg, sizeof(uMsg), 0);
		inc = inc + 1;

		//char* buffer = new char[sizeof(uMsg)];
		//memcpy(buffer, &msg, sizeof(uMsg));

		//send(client, buffer, sizeof(uMsg), 0);

		//send(client, (char*)msg.type, sizeof(int), 0);
		//send(client, (char*)msg.length, sizeof(int), 0);
		//send(client, msg.content, msg.length, 0);
		//sendto(client, content.c_str(), sizeof(content), 0, (sockaddr *)&addrServer, sizeof(addrServer));
		//send(client, content.c_str(), sizeof(content), 0);
		//char temp[1000];
		//uMsg msg;
		//mPoint point;
		//point.x = 100;
		//point.y = 200;
		//msg.m_point = &point;
		//msg.type = 1;
		//strncpy(msg.name, "zhangsan", sizeof(msg.name));
		//strncpy(msg.text, "123456", sizeof(msg.text));

		//memset(temp, 0, sizeof(temp));
		//memcpy(temp, &msg, sizeof(uMsg));
		//send(client, temp, sizeof(uMsg), 0);

		Sleep(1000);
	}
	//getchar();
	//	std::string content;
	//	getline(std::cin, content);
	//	send(client, content.c_str(), sizeof(content), 0);

	//	/*if (content == "quit") {
	//		msg.type = 2;
	//		send(client, (char*)&msg, sizeof(msg), 0);
	//		error_send = GetLastError();
	//		if (error_send != 0) {
	//			std::cout << "send failed:" << error_send << std::endl;
	//		}
	//		closesocket(client);
	//		WSACleanup();
	//		return 0;
	//	}
	//	
	//	msg.type = 3;
	//	strncpy_s(msg.text, sizeof(msg.text), content.c_str(), 512);
	//	send(client, (char*)&msg, sizeof(msg), 0);
	//	error_send = GetLastError();
	//	if (error_send != 0)
	//	{
	//		std::cout << "send failed: " << error_send << std::endl;
	//	}*/
	//}

	//getchar();
	//closeso(client);
	//closesocket(client);
	return 0;
}


