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


#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 256

int main()
{
	std::cout << "****************\n*    CLIENT    *\n****************\n\n";

	// Initialize Winsock
	WSADATA wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		std::cout << "WSAStartup Failed with error: " << iResult << std::endl;
		return 1;
	}

	SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &addrServer.sin_addr.s_addr);
	addrServer.sin_port = htons(6666);
	memset(&(addrServer.sin_zero), '\0', 8);

	std::cout << "Connecting..." << std::endl;
	iResult = connect(ConnectSocket, (SOCKADDR *)&addrServer, sizeof(addrServer));
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		std::cout << "Unable to connect to server: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}


	char filename[DEFAULT_BUFLEN] = { 0 };
	std::cout << "Name of file: ";
	std::cin.getline(filename, DEFAULT_BUFLEN, '\n');

	const char *sendbuf = filename;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Send file name
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	do {
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			std::string size;
			for (int i = 0; i < iResult; i++) size += recvbuf[i];

			if (size == "-1") {
				std::cout << "No file named \"" << filename << "\"" << std::endl;
			}
			else {
				std::cout << "The size of file \"" << filename << "\" is: " << size << std::endl;
			}
		}
		else if (iResult == 0) {
			std::cout << "Connection closed\n" << std::endl;
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
		}
	
	} while (iResult > 0);


	// shutdown the connection
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 1;
}


