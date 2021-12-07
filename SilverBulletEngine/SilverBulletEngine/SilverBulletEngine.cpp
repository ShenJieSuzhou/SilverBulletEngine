// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
//#include <sys/types.h>
#include <winsock2.h>
//#include <winsock.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <thread>
//#include <cstdio>

//#include <stdlib.h>
//#include <stdio.h>
//#include <fcntl.h>
#include <errno.h>
//#include <string.h>
//#include <fstream>
//#include <event.h>
//#include <signal.h>
//#include <stdarg.h>

//#include "WorkQueue.h"

//using namespace std;

#define SERVER_PORT 5555

#define CONNECTION_BACKLOG 8

#define SOCKET_READ_TIMEOUT_SECONDS 10

#define SOCKET_WRITE_TIMEOUT_SECONDS 10

#define NUM_THREADS 8

#define errorOut(...) {\
	fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
}

#pragma comment (lib, "Ws2_32.lib")


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
	//mPoint *m_point;
};

// Client
struct clientInfo 
{
	SOCKET client;
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

// Recv msg from server
void recvMessage(PVOID pParam)
{
	clientInfo *cInfo = (clientInfo *)pParam;

	while (1) {
		// accept message from client
		uMsg msg;
		int len_client = sizeof(sockaddr);
		int ret_recv = recv(cInfo->client, (char*)&msg, sizeof(msg), 0);

		if (ret_recv <= 0)
		{
			std::cout << msg.name << " Disconnect：" << GetLastError() << std::endl;
			break;
		}

		cInfo->msg = msg;

		char arr_ip[20];
		inet_ntop(AF_INET, &cInfo->saddr.sin_addr, arr_ip, 16);

		// Process message
		switch (cInfo->msg.type)
		{
		case 1:
			// Insert data to chain 
			insertNode(listHead, cInfo->client, cInfo->saddr, cInfo->msg);
			std::cout << "[" << arr_ip << ":" << ntohs(cInfo->saddr.sin_port) << "]" << msg.name << ":" << "--- Login---" << std::endl;
			break;
		case 2:
			deleteNode(listHead, cInfo->client);
			std::cout << "[" << arr_ip << ":" << ntohs(cInfo->saddr.sin_port) << "]" << msg.name << ":" << "--- Login Out---" << std::endl;
			break;
		case 3:
			std::cout << "[" << arr_ip << ":" << ntohs(cInfo->saddr.sin_port) << "]" << msg.name << ":" << cInfo->msg.text << std::endl;
			lp = listHead;
			while (lp)
			{
				if (strcmp(lp->cInfo.msg.name, "") != 0 && strcmp(lp->cInfo.msg.name, cInfo->msg.name) != 0)
				{
					send(lp->cInfo.client, (char*)&cInfo->msg, sizeof(cInfo->msg), 0);
					int error_send = GetLastError();
					if (error_send != 0) {
						std::cout << "send failed:" << error_send << std::endl;
					}
				}
				lp = lp->next;
			}
			break;
		default:
			break;
		}
	}

}

#pragma endregion



typedef struct client {
	int fd;

	struct event_base *evbase;
	
	struct bufferevent *buf_ev;
	
	struct evbuffer *output_buffer;
} client_t;


static struct event_base *evbase_accept;
//static workqueue_t workqueue;

static void sigHandler(int signal);


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
#ifdef SEL_LIBEVENT
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
#endif // SEL_LIBEVENT
}

void buffered_on_write(struct bufferevent *bev, void *arg) {

}

void buffered_on_error(struct bufferevent *bev, short what, void *ctx) {
	closeClient((client_t *)ctx);
}
 
static void server_job_function(struct job *job) {
#ifdef SEL_LIBEVENT
	client_t *client = (client_t *)job->user_data;
	event_base_dispatch(client->evbase);
	closeAndFreeClient(client);
	free(job);
#endif // SEL_LIBEVENT
}


// Ready to be accept
void on_accept(int fd, short ev, void *arg) {
#ifdef SEL_LIBEVENT
	int client_fd;
	struct sockaddr_in client_addr;

	socklen_t client_len = sizeof(client_addr);
	workqueue_t *workqueue = (workqueue_t *)arg;
	client_t *client;

	job_t *job;

	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);

	if (client_fd < 0) {
		perror("accept failed");
		return;
	}

	if (evutil_make_socket_nonblocking(client_fd) < 0) {
		perror("failed to set client socket to non-blocking");
		return;
	}
	
	// Create a client object
	if ((client = (client_t *)malloc(sizeof(*client))) == NULL) {
		perror("failed to allocate memory for client state");
		evutil_closesocket(client_fd);
		return;
	}
	
	memset(client, 0, sizeof(*client));
	client->fd = client_fd;

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

	if ((job = (job_t *)malloc(sizeof(*job))) == NULL) {
		perror("failed to allocate memory for job state");
		closeAndFreeClient(client);
		return;
	}

	job->job_function = server_job_function;
	job->user_data = client;

	workqueue_add_job(workqueue, job);
#endif // SEL_LIBEVENT
}

int runServer() {
#ifdef SEL_LIBEVENT
	int listenfd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;
	int reuseaddr_on;

	// Initialize libevent
	event_init();

	// Set signal handler
	typedef void(*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGABRT, sigHandler);

	// Create our listening socket 
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("listen failed");
		return 1;
	}

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

	reuseaddr_on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuseaddr_on, sizeof(reuseaddr_on));

	if (evutil_make_socket_nonblocking(listenfd) < 0) {
		perror("failed to set server socket to non-blocking");
		return 1;
	}

	if ((evbase_accept == event_base_new()) == NULL) {
		perror("Unable to create socket accept event base");
		evutil_closesocket(listenfd);
		return 1;
	}

	// Init workqueue
	if (workqueue_init(&workqueue, NUM_THREADS)) {
		perror("Failed to create work queue");
		evutil_closesocket(listenfd);
		workqueue_shutdown(&workqueue);
		return 1;
	}

	
	event_set(&ev_accept, listenfd, EV_READ | EV_PERSIST, on_accept, (void *)&workqueue);
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
#endif // SEL_LIBEVENT
	return 0;
}


void killServer() {
#ifdef SEL_LIBEVENT
	fprintf(stdout, "Stopping socket listener event loop.\n");
	if (event_base_loopexit(evbase_accept, NULL)) {
		perror("Error shutting down server");
	}
	fprintf(stdout, "Stopping workers.\n");
	workqueue_shutdown(&workqueue);
#endif // SEL_LIBEVENT
}

static void sigHandler(int signal) {
	fprintf(stdout, "Received signal %d.  Shutting down.\n", signal);
	killServer();
}

int main() {
	std::cout << "I am the server" << std::endl;
	
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		std::cout << "WSAStartup failed :" << GetLastError() << std::endl;
	}

	// Create our listening socket 
	int listenfd;
	struct sockaddr_in listen_addr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("listen failed");
		return 1;
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(SERVER_PORT);
	
	int iResult = bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
	if(iResult == SOCKET_ERROR) {
		perror("bind failed");
		WSACleanup();
		return 1;
	}

	// listen
	listen(listenfd, SOMAXCONN);

	listHead = new userClientNode();
	listHead->next = nullptr;
	lp = listHead;
	
	// recv msg
	while (1)
	{
		clientInfo *cInfo = new clientInfo();
		int len_client = sizeof(sockaddr);
		cInfo->client = accept(listenfd, (sockaddr*)&cInfo->saddr, &len_client);

		if (GetLastError() != 0) {
			continue;
		}

		// recv login user info
		HANDLE h_recvMes = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)recvMessage, cInfo, 0, 0);
		if (!h_recvMes) {
			std::cout << "Create Thread failed :" << GetLastError() << std::endl;
		}
	}

	WSACleanup();
	getchar();

	return 0;
}