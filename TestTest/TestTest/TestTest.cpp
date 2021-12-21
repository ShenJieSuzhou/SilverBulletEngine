// TestTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <event.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <thread>
#include <cstdio>
#include <string>

using namespace std;

typedef struct msg_header {
	unsigned int targetID;
	unsigned int sourceID;
	unsigned int length;
} Header;

#define PORT 5555
#define BUFFER_SIZE (16 * 1024)
#define READ_SIZE (16 *1024)
#define SEND_INTERVAL 0
#define CLIENT_NUMBER 2
#define SEND_TIMES 10
#define MAX_PACKET_SIZE 1024
#define HEADER_LENGTH 12
#define DATA_LENGTH 1000

const char ip_address[] = "127.0.0.1";

void conn_writecb(struct bufferevent *bev, void *);
void conn_readcb(struct bufferevent *bev, void *);
void conn_eventcb(struct bufferevent *bev, short, void *);
string inttostr(int);
void write_buffer(string&, struct bufferevent*, Header&);

int main()
{
	cout << "Client running" << endl;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	struct sockaddr_in srv;
	memset(&srv, 0, sizeof(srv));
	srv.sin_addr.s_addr = htonl(0x7f000001);
	srv.sin_family = AF_INET;
	srv.sin_port = htons(PORT);

	struct event_base *base = event_base_new();
	if (!base) {
		cout << "Could not initialize libevent" << endl;
		return 1;
	}

	struct bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);

	//连接，成功返回0，失败返回-1
	int flag = bufferevent_socket_connect(bev, (struct sockaddr *)&srv, sizeof(srv));

	bufferevent_enable(bev, EV_READ | EV_WRITE);

	if (-1 == flag) {
		cout << "Connect failed" << endl;
		return 1;
	}

	event_base_dispatch(base);
	event_base_free(base);

	// Send Message
	//while (1)
	//{
	//	cout << "Send Message" << endl;
	//	std::string content;
	//	getline(std::cin, content);
	//	int len = 0;
	//	static int num[10] = { 0 };
	//	unsigned int toid = rand() % 200;
	//	Header header;
	//	header.targetID = toid;
	//	header.length = content.size();
	//	
	//	write_buffer(content, bev, header);

	//	int error_send = GetLastError();
	//	if (error_send != 0)
	//	{
	//		std::cout << "send failed: " << error_send << std::endl;
	//		//closesocket(client);
	//		WSACleanup();
	//		return 0;
	//	}
	//}

	//getchar();

	return 0;
}

void conn_writecb(struct bufferevent *bev, void *user_data) {
	//Sleep(SEND_INTERVAL);
	//int len = 0;
	//static int num[10] = { 0 };
	//unsigned int toid = rand() % 200 + 1;
	//Header header;

	//string msg = "hello";

	//header.targetID = toid;
	//header.length = msg.size();
	//write_buffer(msg, bev, header);
	//bufferevent_write(bev, "", 10);
	/*while (true)
	{
		cout << "conn_writecb " << endl;
		std::string content;
		getline(std::cin, content);
		int len = 0;
		static int num[10] = { 0 };
		unsigned int toid = rand() % 200;
		Header header;
		header.targetID = toid;
		header.length = content.size();
		
		write_buffer(content, bev, header);
	}*/
}

void write_buffer(string& msg, struct bufferevent* bev, Header& header) {
	char send_msg[READ_SIZE] = { 0 };
	char* ptr = send_msg;
	int len = 0;

	//多发几次，便于测试大量数据
	/*
	for (int i = 0; i < SEND_TIMES; i++) {
		memcpy(ptr + len, &header, sizeof(Header));
		len += sizeof(Header);
		memcpy(ptr + len, msg.c_str(), msg.size());
		len += msg.size();
	}*/
	memcpy(ptr + len, &header, sizeof(Header));
	len += sizeof(Header);
	memcpy(ptr + len, msg.c_str(), msg.size());
	len += msg.size();
	if (bufferevent_write(bev, send_msg, len) < 0) {
		cout << "data send error" << endl;
	}
}

void conn_readcb(struct bufferevent *bev, void *user_data) {
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);	//返回evbuffer储存的字节数

	char buf[255];
	bufferevent_read(bev, buf, sizeof(buf));
	buf[sz] = '\0';
	cout << "收到服务端来消息" << endl;
	fprintf(stdout, "Read: %s\n", buf);

	//while (sz >= MAX_PACKET_SIZE) {
		//char msg[MAX_PACKET_SIZE] = { 0 };
		//char *ptr = msg;
		//bufferevent_read(bev, ptr, HEADER_LENGTH);

		//unsigned int len = ((Header*)ptr)->length;
		//unsigned int targetID = ((Header*)ptr)->targetID;
		//unsigned int sourceID = ((Header*)ptr)->sourceID;

		//ptr += HEADER_LENGTH;

		//if (sz < len + HEADER_LENGTH) {
		//	return;
		//	//break;
		//}

		//bufferevent_read(bev, ptr, len);

		//cout << "receive " << HEADER_LENGTH + strlen(ptr) << " bytes from client " << sourceID << endl;

		//sz = evbuffer_get_length(input);
	//}
}



void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF) {
		cout << "Connection closed" << endl;
	}
	else if (events & BEV_EVENT_ERROR) {
		cout << "Got an error on the connection: " << endl;
	}
	else if (events & BEV_EVENT_CONNECTED) {
		cout << "Connect succeed" << endl;

		string msg = "connect to server";
		/*Header header;
		header.targetID = 0;
		header.length = msg.size();

		write_buffer(msg, bev, header);*/

		if (bufferevent_write(bev, msg.c_str(), sizeof(msg)) < 0) {
			cout << "data send error" << endl;
		}
		return;
	}

	bufferevent_free(bev);
}


string inttostr(int num) {
	string result;
	bool neg = false;
	if (num < 0) {
		neg = true;
		num = -num;
	}
	if (num == 0) {
		result += '0';
	}
	else {
		while (num > 0) {
			int rem = num % 10;
			result = (char)(rem + '0') + result;
			num /= 10;
		}
	}
	if (neg) {
		result = '-' + result;
	}
	return result;
}

