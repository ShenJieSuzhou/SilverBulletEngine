// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

//#include <iostream>
//#include <sys/types.h>
//#include <winsock2.h>
//#include <winsock.h>
//#include <ws2tcpip.h>
//#include <Windows.h>
//#include <thread>
//#include <cstdio>
//#include <stdlib.h>
//#include <stdio.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <string.h>
//#include <fstream>
//#include <event.h>
//#include <signal.h>
//#include <stdarg.h>

//#include "WorkQueue.h"

//using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <thread>

#define SERVER_PORT 5555

#define CONNECTION_BACKLOG 8

#define SOCKET_READ_TIMEOUT_SECONDS 10

#define SOCKET_WRITE_TIMEOUT_SECONDS 10

#define NUM_THREADS 8

#define errorOut(...) {\
	fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
}

//#pragma comment (lib, "Ws2_32.lib")


#pragma region  LogicDemo1
 
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
	char text[512]; // text msg
	mPoint *m_point;
};

// Client
struct clientInfo 
{
	evutil_socket_t client;
	sockaddr_in saddr;
	uMsg msg;
};

// client chain node
typedef struct userClientNode
{
	clientInfo cInfo;
	userClientNode *next;
} *ucnode_t;

userClientNode *listHead;
userClientNode *lp;

static struct event_base *evbase_accept;

#pragma endregion


#pragma region chain node logic

// Insert Client to chain
userClientNode *insertNode(userClientNode *head, SOCKET client, sockaddr_in addr, uMsg msg)
{
	userClientNode *newNode = new userClientNode();
	newNode->cInfo.client = client;
	newNode->cInfo.saddr = addr;
	newNode->cInfo.msg = msg;
	userClientNode *p = head;

	if (p == nullptr)
	{	
		head = newNode;
	}
	else {
		while (p->next != nullptr)
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

	if (p->cInfo.client == client)
	{
		head = p->next;
		delete p;
		return head;
	}

	while (p->next != nullptr && p->next->cInfo.client != client)
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







static void closeClient(client_t *client) {
#ifdef SEL_LIBEVENT
	if (client != NULL) {
		if (client->fd >= 0) {
			evutil_closesocket(client->fd);
			client->fd = -1;
		}
	}
#endif // SEL_LIBEVENT
}


static void closeAndFreeClient(client_t *client) {
#ifdef SEL_LIBEVENT
	if (client != NULL) {
		closeClient(client);
		if (client->buf_ev != NULL) {
			bufferevent_free(client->buf_ev);
			client->buf_ev = NULL;
		}

		if (client->evbase != NULL) {
			event_base_free(client->evbase);
			client->evbase = NULL;
		}
		if (client->output_buffer != NULL) {
			evbuffer_free(client->output_buffer);
			client->output_buffer = NULL;
		}
		free(client);
	}
#endif // SEL_LIBEVENT
}

void buffered_on_read(struct bufferevent *bev, void *arg) {
	client_t *client = (client_t *)arg;
	char data[4096];
	int nbytes;

	while ((nbytes = EVBUFFER_LENGTH(bev->input)) > 0) {
		if (nbytes > 4096) nbytes = 4096;
		evbuffer_remove(bev->input, data, nbytes);
		evbuffer_add(client->output_buffer, data, nbytes);
	}

	if (bufferevent_write_buffer(bev, client->output_buffer)) {
		errorOut("Error sending data to client on fd %d\n", client->fd);
		closeClient(client);
	}
}

void buffered_on_write(struct bufferevent *bev, void *arg) {

}

void buffered_on_error(struct bufferevent *bev, short what, void *ctx) {
	closeClient((client_t *)ctx);
}
 

// Ready to be accept
void on_accept(int fd, short ev, void *arg) 
{
	evbase_accept = (struct event_base *)arg;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	// Create a client object
	clientInfo *cInfo = new clientInfo();
	int len_client = sizeof(sockaddr);
	if (cInfo == NULL)
	{
		perror("failed to allocate memory for client state");
		evutil_closesocket(client_fd);
		return;
	}
	
	cInfo->client = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) 
	{
		perror("accept failed");
		return;
	}

	if (evutil_make_socket_nonblocking(cInfo->client) < 0)
	{
		perror("failed to set client socket to non-blocking");
		return;
	}

	

	if(cInfo->client > FD_SETSIZE)
	{
		perror("client_fd > FD_SETSIZE\n");
		return;
	}

	printf("ACCEPT: fd = %u\n", cInfo->client);


	if ((client->output_buffer = evbuffer_new()) == NULL) {
		perror("client output buffer allocation failed");
		closeAndFreeClient(client);
		return;
	}


	if ((client->evbase = event_base_new()) == NULL) {
		perror("client event_base creation failed");
		closeAndFreeClient(client);
		return;
	}

	// Create buffer event 
	if ((client->buf_ev = bufferevent_new(client_fd, buffered_on_read, buffered_on_write, buffered_on_error, client)) == NULL) {
		perror("client bufferevent creation failed");
		closeAndFreeClient(client);
		return;
	}

	bufferevent_base_set(client->evbase, client->buf_ev);
	bufferevent_settimeout(client->buf_ev, SOCKET_READ_TIMEOUT_SECONDS, SOCKET_WRITE_TIMEOUT_SECONDS);

	bufferevent_enable(client->buf_ev, EV_READ);
}

int runServer() {

	evutil_socket_t listenfd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;

	// Create our listening socket 
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("listen failed");
		return 1;
	}

	evutil_make_listen_socket_reuseable(listenfd);

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(SERVER_PORT);

	if (bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
		perror("bind failed");
		return 1;
	}

	if (listen(listenfd, CONNECTION_BACKLOG) < 0) {
		perror("listen failed");
		return 1;
	} 

	printf("Listening....")；

	if (evutil_make_socket_nonblocking(listenfd) < 0) {
		perror("failed to set server socket to non-blocking");
		return 1;
	}
	
	if ((evbase_accept = event_base_new()) == NULL) {
		perror("Unable to create socket accept event base");
		evutil_closesocket(listenfd);
		return 1;
	}

	event_set(&ev_accept, listenfd, EV_READ | EV_PERSIST, on_accept, (void *)evbase_accept);
	event_base_set(evbase_accept, &ev_accept);
	event_add(&ev_accept, NULL);

	printf("Server running. \n");

	// Start the event loop
	event_base_dispatch(evbase_accept);

	event_base_free(evbase_accept);

	evbase_accept = NULL;
	
	evutil_closesocket(listenfd);

	printf("Server shutdown.\n");

	return 0;
}


void killServer() {
	fprintf(stdout, "Stopping socket listener event loop.\n");
	if (event_base_loopexit(evbase_accept, NULL) < 0) {
		perror("Error shutting down server");
	}
	fprintf(stdout, "Stopping workers.\n");
}

static void sigHandler(int signal) {
	fprintf(stdout, "Received signal %d.  Shutting down.\n", signal);
	killServer();
}

int main() {
	return runServer();
}