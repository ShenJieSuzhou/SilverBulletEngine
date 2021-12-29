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
#include <thread>

using namespace std;

typedef struct msg_header {
	unsigned int targetID;
	unsigned int sourceID;
	unsigned int length;
} Header;

#define PORT 9988
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
void startChat(void *arg);

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

	//HANDLE h_recvMes = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)startChat, (void *)bev, 0, 0);
	//if (!h_recvMes) {
	//	std::cout << "Create thread failed : " << GetLastError() << std::endl;
	//	return 0;
	//}
	std::thread((LPTHREAD_START_ROUTINE)startChat, (void *)bev).detach();

	event_base_dispatch(base);
	event_base_free(base);

	return 0;
}

void conn_writecb(struct bufferevent *bev, void *user_data) {

}

void write_buffer(string& msg, struct bufferevent* bev, Header& header) {
	//char send_msg[READ_SIZE] = { 0 };
	//char* ptr = send_msg;
	//int len = 0;

	//多发几次，便于测试大量数据
	/*
	for (int i = 0; i < SEND_TIMES; i++) {
		memcpy(ptr + len, &header, sizeof(Header));
		len += sizeof(Header);
		memcpy(ptr + len, msg.c_str(), msg.size());
		len += msg.size();
	}*/
	//memcpy(ptr + len, &header, sizeof(Header));
	//len += sizeof(Header);
	//memcpy(ptr + len, msg.c_str(), msg.size());
	//len += msg.size();
	//if (bufferevent_write(bev, send_msg, len) < 0) {
	//	cout << "data send error" << endl;
	//}
}

void conn_readcb(struct bufferevent *bev, void *user_data) {
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);	//返回evbuffer储存的字节数

	char buf[255];
	bufferevent_read(bev, buf, sizeof(buf));
	buf[sz] = '\0';
	cout << "收到服务端来消息" << endl;
	fprintf(stdout, "Read: %s\n", buf);
}
//
//void set_tcp_no_delay(evutil_socket_t fd)
//{
//	int one = 1;
//	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
//	int reuseaddr_on = 1;
//	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
//		sizeof(reuseaddr_on));
//}


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
		//evutil_socket_t fd = bufferevent_getfd(bev);
		//set_tcp_no_delay(fd);
		std::string content = "hello ";
		bufferevent_write(bev, content.c_str(), sizeof(content));
		return;
	}

	bufferevent_free(bev);
}

void startChat(void *arg)
{
	struct bufferevent *bev = (struct bufferevent *)arg;
	std::string content;
	//Send msg
	while (getline(std::cin, content))
	{

		bufferevent_write(bev, content.c_str(), sizeof(content));
		//content = "Tom:" + content;
		//evbuffer_add(bufferevent_get_output(bev), content.c_str(), content.size());
	}
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

