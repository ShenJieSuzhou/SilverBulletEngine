// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 256


int main()
{
	std::cout << "****************\n*    SERVER    *\n****************\n\n";
	

	char str[INET_ADDRSTRLEN];

	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		std::cout << "WSAStartup failed with error:" << iResult << std::endl;
		return 1;
	}

	// Create a socket to listening the incoming connection requests
	SOCKET ListenSocket, ClientSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		std::cout << "Socket failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}


	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &addrServer.sin_addr.s_addr);
	addrServer.sin_port = htons(6666);
	memset(&(addrServer.sin_zero), '\0', 8);

	// Bind socket
	if (bind(ListenSocket, (SOCKADDR *)& addrServer, sizeof(addrServer)) == SOCKET_ERROR) {
		std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	

	// Listen for incoming connection
	if (listen(ListenSocket, 5) == SOCKET_ERROR){
		std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Variables for receive
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive data
	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		std::string filename;

		for (int i = 0; i < iResult; i++){
			filename += recvbuf[i];
		}

		//std::fstream file;
		//file.open(filename, std::ios::in | std::ios::binary);

		//file.seekg(0, std::ios::end);
		//int fileSize = file.tellg();
		//file.close();
		std::cout << "File name: " << filename << std::endl;


		std::string temp = filename;
		char tempc[DEFAULT_BUFLEN];
		int i = 0;
		while (temp[i] != '\0')
		{	
			tempc[i] = temp[i];
			i++;
		}
		tempc[i] = '\0';

		const char* sendbuf = tempc;

		// send file size to client
		iSendResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
		if (iSendResult == SOCKET_ERROR) {
			std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	}
	else if (iResult == 0) {
		std::cout << "Connection closing... \n" << std::endl;
	}
	else {
		std::cout << "Recieve failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Connection closing...\n" << std::endl;

	//Shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}

