// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

//#undef UNICODE

//#define WIN32_LEAN_AND_MEAN

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

#define SERVER_PORT 5555

#define CONNECTION_BACKLOG 8

#define SOCKET_READ_TIMEOUT_SECONDS 10

#define SOCKET_WRITE_TIMEOUT_SECONDS 10

#define NUM_THREADS 8

#pragma comment (lib, "Ws2_32.lib")

//struct mPoint
//{
//	int x; 
//	int y;
//};

// Message
//struct uMsg
//{
//	int type;
//	//char name[64];
//	char text[512]; // text msg
//	//mPoint *m_point;
//};

// Client
//struct clientInfo 
//{
//	evutil_socket_t fd;
//	sockaddr_in saddr;
//	uMsg msg;
//};

typedef struct userClientNode
{
	evutil_socket_t fd;

	struct event_base *evbase;

	struct bufferevent *buf_ev;

	struct evbuffer *output_buffer;

	userClientNode *next;

} *ucnode_t;


//// client chain node
//typedef struct userClientNode
//{
//	client cInfo;
//	userClientNode *next;
//} *ucnode_t;

userClientNode *listHead;
userClientNode *lp;

static struct event_base *evbase_accept;

static char g_szReadMsg[256] = { 0 };



// Insert Client to chain
userClientNode *insertNode(userClientNode *head, SOCKET client, struct event_base *evbase, struct bufferevent *buf_ev, struct evbuffer *output_buffer)
{
	userClientNode *newNode = new userClientNode();
	newNode->fd = client;
	newNode->evbase = evbase;
	newNode->buf_ev = buf_ev;
	newNode->output_buffer = output_buffer;
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


static void closeClient(userClientNode *cli)
{
	if (cli != NULL) {
		if (cli->fd >= 0) {
			evutil_closesocket(cli->fd);
			cli->fd = -1;
		}
	}
}

void buffered_on_read(struct bufferevent *bev, void *arg) 
{
	struct userClientNode *this_client = (struct userClientNode *)arg;
	uint8_t data[8192];
	size_t n;

	memset(g_szReadMsg, 0x00, sizeof(g_szReadMsg));
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);

	if (sz > 0)
	{
		bufferevent_read(bev, g_szReadMsg, sz);
		printf("ser:>>%s\n", g_szReadMsg);
	}
}

void buffered_on_write(struct bufferevent *bev, void *arg) 
{

}

void buffered_on_error(struct bufferevent *bev, short event, void *arg) 
{
	evutil_socket_t fd = bufferevent_getfd(bev);
	printf("fd = %u, ", fd);
	struct userClientNode *cli = (struct userClientNode *)arg;
	if (event & BEV_EVENT_TIMEOUT) 
	{
		printf("Timed out\n"); //if bufferevent_set_timeouts() called
	}
	else if (event & BEV_EVENT_EOF) 
	{
		printf("connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR) 
	{
		printf("some other error\n");
	}

	bufferevent_free(cli->buf_ev);
	// Delete node from queue
	deleteNode(listHead, cli->fd);

	closeClient(cli);
	evbase_accept = NULL;
	free(cli);
}
 

// Ready to be accept
void on_accept(evutil_socket_t fd, short ev, void *arg)
{
	struct event_base *base = (struct event_base *)arg;
	evutil_socket_t client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0)
	{
		perror("accept failed");
		return;
	}

	if (evutil_make_socket_nonblocking(client_fd) < 0)
	{
		perror("failed to set client socket to non-blocking");
		return;
	}

	// Create a client object
	userClientNode *cInfo = new userClientNode();
	int len_client = sizeof(sockaddr);
	if (cInfo == NULL)
	{
		perror("failed to allocate memory for client state");
		evutil_closesocket(client_fd);
		return;
	}

	cInfo->fd = client_fd;

	printf("Accepted connection from: fd = %u\n", cInfo->fd);

	if ((cInfo->output_buffer = evbuffer_new()) == NULL) {
		perror("client output buffer allocation failed");
		//closeAndFreeClient(client);
		return;
	}


	if ((cInfo->evbase = event_base_new()) == NULL) {
		perror("client event_base creation failed");
		//closeAndFreeClient(client);
		return;
	}

	// Create buffer event 
	struct bufferevent *bev = bufferevent_socket_new(base, cInfo->fd, BEV_OPT_CLOSE_ON_FREE);
	if (bev == NULL)
	{
		perror("client bufferevent creation failed");
		return;
	}
	bufferevent_setcb(bev, buffered_on_read, buffered_on_write, buffered_on_error, arg);
	cInfo->buf_ev = bev;
	bufferevent_base_set(cInfo->evbase, cInfo->buf_ev);
	bufferevent_settimeout(cInfo->buf_ev, SOCKET_READ_TIMEOUT_SECONDS, SOCKET_WRITE_TIMEOUT_SECONDS);
	bufferevent_enable(cInfo->buf_ev, EV_READ | EV_WRITE | EV_PERSIST);

	insertNode(listHead, cInfo->fd, cInfo->evbase, cInfo->buf_ev, cInfo->output_buffer);

	// send a message to client when connect is succeeded
	char send_msg[1024] = { 0 };
	std::string msg = "connected";
	//char *ptr = send_msg;
	//memcpy(ptr, msg.c_str(), msg.size);
	int len = msg.size();
	int result = bufferevent_write(bev, msg.c_str(), len);
	if (result < 0) 
	{
		perror("Server send error");
	}
}

int runServer() {
	std::cout << "I am the server" << std::endl;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		std::cout << "WSAStartup failed :" << GetLastError() << std::endl;
	}

	int listenfd;
	struct sockaddr_in listen_addr;
	struct event * ev_accept;

	// Initialize the queue
	listHead = new userClientNode();
	listHead->next = nullptr;
	lp = listHead;

	// Create our listening socket 
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		std::cout << "listen failed: " << GetLastError() << std::endl;
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
		perror("listen failed:" );
		std::cout << "listen failed: " << GetLastError() << std::endl;

		return 1;
	} 

	printf("Listening.... \n");

	if (evutil_make_socket_nonblocking(listenfd) < 0) {
		perror("failed to set server socket to non-blocking");
		return 1;
	}
	
	if ((evbase_accept = event_base_new()) == NULL) {
		perror("Unable to create socket accept event base");
		evutil_closesocket(listenfd);
		return 1;
	}

	printf("Server running. \n");

	ev_accept = event_new(evbase_accept, listenfd, EV_READ | EV_PERSIST, on_accept, (void*)evbase_accept);
	event_add(ev_accept, NULL);

	// Start the event loop
	event_base_dispatch(evbase_accept);
	
	return 0;
}


void killServer() {
	fprintf(stdout, "Stopping socket listener event loop.\n");
	if (event_base_loopexit(evbase_accept, NULL) < 0) {
		perror("Error shutting down server");
	}
	fprintf(stdout, "Stopping workers.\n");
}


int main() {
	return runServer();
}