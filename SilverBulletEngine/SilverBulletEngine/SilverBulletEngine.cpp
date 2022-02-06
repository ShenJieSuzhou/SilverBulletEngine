// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <string>

using namespace std;

#define PORT 5555

// Message
struct uMsg
{
	int len;
	int accLen;
	string account;
	int type;
	float x;
	float y;
	float z;
};

struct userClientNode
{
	int fd;
	string client_ip;
	string uuid;
	struct bufferevent *bev;
	//struct uMsg *clientInfo;
	userClientNode *next;
};

userClientNode *listHead;

// Insert Client to chain
userClientNode *insertNode(userClientNode *head, SOCKET client, struct bufferevent *buf_ev, string ip)
{
	userClientNode *newNode = new userClientNode();
	newNode->fd = client;
	newNode->bev = buf_ev;
	newNode->client_ip = ip;
	newNode->next = NULL;
	userClientNode *p = head;

	if (p == NULL)
	{
		head = newNode;
	}
	else
	{
		while (p->next != NULL)
		{
			p = p->next;
		}
		p->next = newNode;
	}

	return head;
}

// Delete node 
userClientNode *deleteNode(userClientNode *head, SOCKET client)
{
	userClientNode *p = head;
	if (p == nullptr)
	{
		return head;
	}

	if (p->fd == client)
	{
		head = p->next;
		delete p;
		return head;
	}

	while (p->next != nullptr && p->next->fd != client)
	{
		p = p->next;
	}

	if (p->next == nullptr)
	{
		return head;
	}

	userClientNode *deleteNode = p->next;
	p->next = deleteNode->next;
	delete deleteNode;
	return head;
}


const char ip_address[] = "127.0.0.1";

void listener_cb(struct evconnlistener *bev, evutil_socket_t,
	struct sockaddr *, int socklen, void *);
void conn_readcb(struct bufferevent *bev, void *);
void conn_eventcb(struct bufferevent *bev, short, void *);
void newUserOnline(userClientNode *node);


//void conn_writecb(struct bufferevent *bev, void *);
//unsigned int get_client_id(struct bufferevent *bev);
//void readHeader(struct bufferevent * bev, struct uMsg* msg);


int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	cout << "Server begin running!" << endl;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	struct evconnlistener *listener;
	struct event_base *base = event_base_new();
	if (!base) {
		cout << "Could not initialize libevent" << endl;
		return 1;
	}
	//默认情况下，链接监听器接收新套接字后，会将其设置为非阻塞的
	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr*)&sin,
		sizeof(sin));

	if (!listener) {
		cout << "Could not create a listener" << endl;
		return 1;
	}

	// Init client list
	//listHead = new userClientNode();
	listHead = NULL;

	event_base_dispatch(base);
	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
}

void listener_cb(struct evconnlistener *listener,
	evutil_socket_t fd,
	struct sockaddr *sa,
	int socklen,
	void *user_data)
{
	cout << "Detect an connection" << endl;
	// Get ip address
	struct sockaddr_in * client_addr = (struct sockaddr_in *)sa;
	char sendBuf[20] = { '\0' };
	inet_ntop(AF_INET, (void*)&client_addr->sin_addr, sendBuf, 16);
	string cli_addr = sendBuf;
	printf("Accepted connection from: %s \n", cli_addr);

	struct event_base *base = (struct event_base *)user_data;
	struct bufferevent *bev;

	if (evutil_make_socket_nonblocking(fd) < 0)
	{
		perror("failed to set client socket to non-blocking");
		return;
	}

	//BEV_OPT_CLOSE_ON_FREE close the file descriptor when this bufferevent is freed
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		cout << "Could not create a bufferevent" << endl;
		event_base_loopbreak(base);
		return;
	}

	// Create a client object
	userClientNode *cInfo = new userClientNode();
	if (cInfo == NULL)
	{
		perror("failed to allocate memory for client state");
		evutil_closesocket(fd);
		return;
	}

	cInfo->fd = fd;
	cInfo->client_ip = sendBuf;
	cInfo->bev = bev;

	// Set read write event
	bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, cInfo);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	// Insert node to client list 
	listHead = insertNode(listHead, cInfo->fd, cInfo->bev, cInfo->client_ip);

	newUserOnline(cInfo);
}

void newUserOnline(userClientNode *node)
{
	// Revise list and send new client ip to old connected clients, also send old connected 
	// clients ip to new client
	userClientNode *newNode = node;
	userClientNode *curr = listHead;

	if (curr == NULL)
	{
		return;
	}

	while (curr != NULL)
	{
		userClientNode *oldNode = curr;
		if (oldNode->client_ip == "" || oldNode->fd == node->fd)
		{
			curr = curr->next;
			continue;
		}

		// Old client ip
		const char *oldIp = oldNode->client_ip.c_str();
		// New client ip
		const char *newIp = newNode->client_ip.c_str();

		// Message length
		uint32_t len = 0;
		len = strlen(oldIp);

		// if type == 200, represent new user online
		int type = 200;
		len = htonl(len);
		type = htonl(type);
		bufferevent_write(node->bev, (char*)&len, sizeof(int));
		bufferevent_write(node->bev, (char*)&type, sizeof(int));
		bufferevent_write(node->bev, oldIp, strlen(oldIp));

		len = strlen(newIp);
		len = htonl(len);
		bufferevent_write(oldNode->bev, (char*)&len, sizeof(int));
		bufferevent_write(oldNode->bev, (char*)&type, sizeof(int));
		bufferevent_write(oldNode->bev, newIp, strlen(newIp));

		curr = curr->next;
	}
}

// Recv message from client
void conn_readcb(struct bufferevent *bev, void *arg) {
	struct userClientInfo *this_client = (userClientInfo *)arg;

	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);
	evutil_socket_t client_fd = bufferevent_getfd(bev);

	int type = 0, len = 0, uuidLen = 0, accLen = 0, color = 0;
	double x = 0.0, y = 0.0;  
	int readSize = 0;

	readSize += bufferevent_read(bev, &len, sizeof(int));
	readSize += bufferevent_read(bev, &type, sizeof(int));

	// 新用户进入房间
	if (type == 201)
	{
		readSize += bufferevent_read(bev, &uuidLen, sizeof(int));
		char *uuid = new char[uuidLen + 1];
		readSize += bufferevent_read(bev, uuid, uuidLen);
		uuid[uuidLen] = '\0';

		readSize += bufferevent_read(bev, &accLen, sizeof(int));

		char *account = new char[accLen + 1];
		readSize += bufferevent_read(bev, account, accLen);
		account[accLen] = '\0';
		
		// color
		readSize += bufferevent_read(bev, &color, sizeof(int));

		len = htonl(len);
		type = htonl(type);
		uuidLen = htonl(uuidLen);
		accLen = htonl(accLen);
		color = htonl(color);

		// Broadcast to other client 
		userClientNode *curr = listHead;
		while (curr != NULL)
		{
			if (curr->fd != client_fd)
			{
				bufferevent_write(curr->bev, (char*)&len, sizeof(int));
				bufferevent_write(curr->bev, (char*)&type, sizeof(int));
				bufferevent_write(curr->bev, (char*)&uuidLen, sizeof(int));
				bufferevent_write(curr->bev, uuid, strlen(uuid));
				bufferevent_write(curr->bev, (char*)&accLen, sizeof(int));
				bufferevent_write(curr->bev, account, strlen(account));
				bufferevent_write(curr->bev, (char*)&color, sizeof(int));
				bufferevent_write(curr->bev, (char*)&x, sizeof(double));
				bufferevent_write(curr->bev, (char*)&y, sizeof(double));
			}
			else 
			{
				curr->uuid = uuid;
			}
			curr = curr->next;
		}

		delete[] uuid;
		uuid = nullptr;

		delete[] account;
		account = nullptr;
	}
	else if (type == 202)
	{
		readSize += bufferevent_read(bev, &uuidLen, sizeof(int));
		char *uuid = new char[uuidLen + 1];
		readSize += bufferevent_read(bev, uuid, uuidLen);
		uuid[uuidLen] = '\0';
		//if (readSize == 12)
		//{
		//	// uuid
		//	uuid = htonl(uuid);
		//}
		readSize += bufferevent_read(bev, &accLen, sizeof(int));
		//if (readSize == 16) {
		// 账号长度
		//accLen = htonl(accLen);
		//}
		char *account = new char[accLen + 1];
		readSize += bufferevent_read(bev, account, accLen);
		account[accLen] = '\0';

		// color
		readSize += bufferevent_read(bev, &color, sizeof(int));

		readSize += bufferevent_read(bev, &x, sizeof(double));
		if (readSize == (24 + accLen))
		{
			// 获得 x 坐标
			//x = htonl(x);
		}
		readSize += bufferevent_read(bev, &y, sizeof(double));
		if (readSize == (32 + accLen))
		{
			// 获得 y 坐标
			//y = htonl(y);
		}

		printf("total data length: %u  uuid: %u acc: %s  坐标：%lf:%lf \n", len, uuid, account, x, y);

		len = htonl(len);
		type = htonl(type);
		uuidLen = htonl(uuidLen);
		accLen = htonl(accLen);
		color = htonl(color);
		// Broadcast to other client 
		userClientNode *curr = listHead;
		while (curr != NULL)
		{
			if (curr->fd != client_fd)
			{
				bufferevent_write(curr->bev, (char*)&len, sizeof(int));
				bufferevent_write(curr->bev, (char*)&type, sizeof(int));
				bufferevent_write(curr->bev, (char*)&uuidLen, sizeof(int));
				bufferevent_write(curr->bev, uuid, strlen(uuid));
				bufferevent_write(curr->bev, (char*)&accLen, sizeof(int));
				bufferevent_write(curr->bev, account, strlen(account));
				bufferevent_write(curr->bev, (char*)&color, sizeof(int));
				bufferevent_write(curr->bev, (char*)&x, sizeof(double));
				bufferevent_write(curr->bev, (char*)&y, sizeof(double));
			}
			curr = curr->next;
		}

		delete[] uuid;
		uuid = nullptr;

		delete[] account;
		account = nullptr;
	}
}


void conn_eventcb(struct bufferevent *bev, short events, void *arg) {
	if (events & BEV_EVENT_EOF) {
		cout << "Connection closed" << endl;

		struct userClientInfo *this_client = (userClientInfo *)arg;
		struct evbuffer *input = bufferevent_get_input(bev);
		size_t sz = evbuffer_get_length(input);
		evutil_socket_t client_fd = bufferevent_getfd(bev);
		int type = 203;
		type = htonl(type);
		userClientNode *curr = listHead;
		while (curr != NULL)
		{
			if ((curr->fd == client_fd) && strlen(curr->uuid.c_str()) > 0)
			{
				bufferevent_write(curr->bev, (char*)&type, sizeof(int));
				int len = strlen(curr->uuid.c_str());
				bufferevent_write(curr->bev, curr->uuid.c_str(), len);
			}
			curr = curr->next;
		}

	}
	else if (events & BEV_EVENT_ERROR) {
		cout << "Got an error on the connection: " << endl;

	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		cout << "Time out\n" << endl;
	}

	evutil_socket_t fd = bufferevent_getfd(bev);
	userClientNode *cli = (userClientNode *)arg;
	bufferevent_free(cli->bev);
	listHead = deleteNode(listHead, cli->fd);

	evutil_closesocket(cli->fd);
	cli->fd = -1;
	free(cli);
}