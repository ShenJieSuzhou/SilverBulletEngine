// SilverBulletEngine.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <event.h>
#include <signal.h>
#include <stdarg.h>

#include "WorkQueue.h"

#define SERVER_PORT 5555

#define CONNECTION_BACKLOG 8

#define SOCKET_READ_TIMEOUT_SECONDS 10

#define SOCKET_WRITE_TIMEOUT_SECONDS 10

#define NUM_THREADS 8

#define errorOut(...) {\
	fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
}


//#pragma comment (lib, "Ws2_32.lib")
//#define DEFAULT_BUFLEN 256


//int main()
//{
//	//std::cout << "****************\n*    SERVER    *\n****************\n\n";
//	//
//
//	//char str[INET_ADDRSTRLEN];
//
//	//// Initialize Winsock
//	//WSADATA wsaData;
//	//int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
//	//if (iResult != NO_ERROR) {
//	//	std::cout << "WSAStartup failed with error:" << iResult << std::endl;
//	//	return 1;
//	//}
//
//	//// Create a socket to listening the incoming connection requests
//	//SOCKET ListenSocket, ClientSocket;
//	//ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	//if (ListenSocket == INVALID_SOCKET) {
//	//	std::cout << "Socket failed with error: " << WSAGetLastError() << std::endl;
//	//	WSACleanup();
//	//	return 1;
//	//}
//
//
//	//sockaddr_in addrServer;
//	//addrServer.sin_family = AF_INET;
//	//InetPton(AF_INET, "127.0.0.1", &addrServer.sin_addr.s_addr);
//	//addrServer.sin_port = htons(6666);
//	//memset(&(addrServer.sin_zero), '\0', 8);
//
//	//// Bind socket
//	//if (bind(ListenSocket, (SOCKADDR *)& addrServer, sizeof(addrServer)) == SOCKET_ERROR) {
//	//	std::cout << "Bind failed with error: " << WSAGetLastError() << std::endl;
//	//	closesocket(ListenSocket);
//	//	WSACleanup();
//	//	return 1;
//	//}
//	//
//
//	//// Listen for incoming connection
//	//if (listen(ListenSocket, 5) == SOCKET_ERROR){
//	//	std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
//	//	closesocket(ListenSocket);
//	//	WSACleanup();
//	//	return 1;
//	//}
//
//	//// Accept a client socket
//	//ClientSocket = accept(ListenSocket, NULL, NULL);
//	//if (ClientSocket == INVALID_SOCKET) {
//	//	std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
//	//	closesocket(ListenSocket);
//	//	WSACleanup();
//	//	return 1;
//	//}
//
//	//// Variables for receive
//	//int iSendResult;
//	//char recvbuf[DEFAULT_BUFLEN];
//	//int recvbuflen = DEFAULT_BUFLEN;
//
//	//// Receive data
//	//iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
//	//if (iResult > 0) {
//	//	std::string filename;
//
//	//	for (int i = 0; i < iResult; i++){
//	//		filename += recvbuf[i];
//	//	}
//
//	//	//std::fstream file;
//	//	//file.open(filename, std::ios::in | std::ios::binary);
//
//	//	//file.seekg(0, std::ios::end);
//	//	//int fileSize = file.tellg();
//	//	//file.close();
//	//	std::cout << "File name: " << filename << std::endl;
//
//
//	//	std::string temp = filename;
//	//	char tempc[DEFAULT_BUFLEN];
//	//	int i = 0;
//	//	while (temp[i] != '\0')
//	//	{	
//	//		tempc[i] = temp[i];
//	//		i++;
//	//	}
//	//	tempc[i] = '\0';
//
//	//	const char* sendbuf = tempc;
//
//	//	// send file size to client
//	//	iSendResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
//	//	if (iSendResult == SOCKET_ERROR) {
//	//		std::cout << "Send failed with error: " << WSAGetLastError() << std::endl;
//	//		closesocket(ClientSocket);
//	//		WSACleanup();
//	//		return 1;
//	//	}
//	//}
//	//else if (iResult == 0) {
//	//	std::cout << "Connection closing... \n" << std::endl;
//	//}
//	//else {
//	//	std::cout << "Recieve failed with error: " << WSAGetLastError() << std::endl;
//	//	closesocket(ClientSocket);
//	//	WSACleanup();
//	//	return 1;
//	//}
//
//	//std::cout << "Connection closing...\n" << std::endl;
//
//	////Shutdown the connection since we're done
//	//iResult = shutdown(ClientSocket, SD_SEND);
//	//if (iResult == SOCKET_ERROR) {
//	//	printf("shutdown failed with error: %d\n", WSAGetLastError());
//	//	closesocket(ClientSocket);
//	//	WSACleanup();
//	//	return 1;
//	//}
//
//	//// cleanup
//	//closesocket(ClientSocket);
//	//WSACleanup();
//
//	return 0;
//}



typedef struct client {
	int fd;

	struct event_base *evbase;
	
	struct bufferevent *buf_ev;
	
	struct evbuffer *output_buffer;
} client_t;


static struct event_base *evbase_accept;
static workqueue_t workqueue;

static void sigHandler(int signal);

static int setNonBlock(int fd) {

}

static void closeClient(client_t *client) {
	if (client != NULL) {
		if (client->fd >= 0) {
			evutil_closesocket(client->fd);
			client->fd = -1;
		}
	}
}


static void closeAndFreeClient(client_t *client) {
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
 
static void server_job_function(struct job *job) {
	client_t *client = (client_t *)job->user_data;
	event_base_dispatch(client->evbase);
	closeAndFreeClient(client);
	free(job);
}


// Ready to be accept
void on_accept(int fd, short ev, void *arg) {
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
}

int runServer() {
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
}


void killServer() {
	fprintf(stdout, "Stopping socket listener event loop.\n");
	if (event_base_loopexit(evbase_accept, NULL)) {
		perror("Error shutting down server");
	}
	fprintf(stdout, "Stopping workers.\n");
	workqueue_shutdown(&workqueue);
}

static void sigHandler(int signal) {
	fprintf(stdout, "Received signal %d.  Shutting down.\n", signal);
	killServer();
}

int main() {

	return runServer();
}