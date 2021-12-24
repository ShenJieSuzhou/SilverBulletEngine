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

#include <vector>
#include <map>
#include <string>


using namespace std;



#define PORT 5555
#define BUFFER_SIZE (16 * 1024)
#define READ_SIZE (16 *1024)
#define TIME_INTERVAL 5000
#define MAX_PACKET_SIZE 1024
#define HEADER_LENGTH 12
#define DATA_LENGTH 1000

typedef struct msg_header {
	unsigned int targetID;
	unsigned int sourceID;
	unsigned int length;
} Header;

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

typedef struct userClientNode
{
	int fd;

	//struct event_base *evbase;

	struct bufferevent *buf_ev;

	//struct evbuffer *output_buffer;

	struct uMsg *clientInfo;

	userClientNode *next;

} *ucnode_t;

userClientNode *listHead;
userClientNode *lp;

// Insert Client to chain
userClientNode *insertNode(userClientNode *head, SOCKET client, struct event_base *evbase, struct bufferevent *buf_ev, struct evbuffer *output_buffer, uMsg *clientInfo)
{
	userClientNode *newNode = new userClientNode();
	newNode->fd = client;
	//newNode->evbase = evbase;
	//newNode->buf_ev = buf_ev;
	//newNode->output_buffer = output_buffer;
	newNode->clientInfo = clientInfo;
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


const char ip_address[] = "127.0.0.1";

void listener_cb(struct evconnlistener *bev, evutil_socket_t,
	struct sockaddr *, int socklen, void *);
void conn_writecb(struct bufferevent *bev, void *);
void conn_readcb(struct bufferevent *bev, void *);
void conn_eventcb(struct bufferevent *bev, short, void *);
unsigned int get_client_id(struct bufferevent *bev);
string inttostr(int);
//void write_buffer(string&, struct bufferevent *bev, Header&);

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
	//listHead->next = NULL;
	//lp = listHead;

	event_base_dispatch(base);
	evconnlistener_free(listener);
	event_base_free(base);

	return 0;
}

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
	struct sockaddr *sa, int socklen, void *user_data) {
	cout << "Detect an connection" << endl;
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

	//read write event
	bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	printf("Accepted connection from: fd = %u\n", fd);

	//if ((cInfo->output_buffer = evbuffer_new()) == NULL) {
	//	perror("client output buffer allocation failed");
	//	return;
	//}


	//if ((cInfo->evbase = event_base_new()) == NULL) {
	//	perror("client event_base creation failed");
	//	return;
	//}

	//if ((cInfo->clientInfo = new uMsg()) == NULL)
	//{
	//	perror("client uMsg creation failed");
	//	return;
	//}

	////send a message to client when connect is succeeded
	//string msg = "connedted";
	//Header header;
	//header.sourceID = 0;
	//header.targetID = conectNumber;
	//header.length = msg.size();
	//write_buffer(msg, bev, header);
}

//// 发消息
//void conn_writecb(struct bufferevent *bev, void *user_data) {
//	Sleep(2000);
//	int len = 0;
//	Header header;
//	unsigned int toID = get_client_id(bev);
//	string msg = "hello client " + inttostr(toID);
//
//	header.targetID = toID;
//	header.sourceID = 0;
//	header.length = msg.size();
//
//	write_buffer(msg, bev, header);
//}

// Recv message from client
void conn_readcb(struct bufferevent *bev, void *user_data) {
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t sz = evbuffer_get_length(input);
	//unsigned int sourceID = get_client_id(bev);




	char buf[255];
	bufferevent_read(bev, buf, sizeof(buf));
	buf[sz] = '\0';
	cout << "收到客户端来消息" << endl;
	fprintf(stdout, "Read: %s\n", buf);
	string msg = "I am server";
	
	// Broadcast msg to all connected client except self
	if (bufferevent_write(bev, msg.c_str(), sizeof(msg)) < 0) {
		cout << "server write error" << endl;
	}
	
	// Create a client object
	/*userClientNode *cInfo = new userClientNode();
	cInfo->next = NULL;
	if (cInfo == NULL)
	{
		perror("failed to allocate memory for client state");
		evutil_closesocket(fd);
		return;
	}

	cInfo->fd = fd;*/

	//while (sz >= MAX_PACKET_SIZE) {
//	char msg[MAX_PACKET_SIZE] = { 0 };
//	char *ptr = msg;
//	bufferevent_read(bev, ptr, HEADER_LENGTH);


//	unsigned int len = ((Header*)ptr)->length;
//	unsigned int targetID = ((Header*)ptr)->targetID;
//	((Header*)ptr)->sourceID = sourceID;

//	ptr += HEADER_LENGTH;

//	if (sz < len + HEADER_LENGTH) {
//		return;
//		//break;
//	}

//	bufferevent_read(bev, ptr, len);

//	receiveNumber++;
//	dataSize += len + HEADER_LENGTH;

//	if (ClientMap.find(targetID) != ClientMap.end()) {
//		sendNumber++;
//		//bufferevent_write(ClientMap[targetID], msg, len + HEADER_LENGTH);
//	}
//	else {
//		//can't find
//	}
//	sz = evbuffer_get_length(input);
//}

////calculate the speed of data and packet
//clock_t nowtime = clock();
//if (lastTime == 0) {
//	lastTime = nowtime;
//}
//else {
//	cout << "client number: " << ClientMap.size() << " ";
//	cout << "data speed: " << (double)dataSize / (nowtime - lastTime) << "k/s ";
//	cout << "packet speed: receive " << (double)receiveNumber / (nowtime - lastTime) << "k/s ";
//	cout << "send " << (double)sendNumber / (nowtime - lastTime) << "k/s" << endl;
//	if (nowtime - lastTime > TIME_INTERVAL) {
//		dataSize = 0;
//		lastTime = nowtime;
//		receiveNumber = 0;
//		sendNumber = 0;
//	}
//}
}


void conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
	if (events & BEV_EVENT_EOF) {
		cout << "Connection closed" << endl;
	}
	else if (events & BEV_EVENT_ERROR) {
		cout << "Got an error on the connection: " << endl;
	}

	bufferevent_free(bev);
}

//unsigned int get_client_id(struct bufferevent* bev) {
//	for (auto p = ClientMap.begin(); p != ClientMap.end(); p++) {
//		if (p->second == bev) {
//			return p->first;
//		}
//	}
//	return 0;
//}

void write_buffer(string& msg, struct bufferevent* bev, Header& header) {
	char send_msg[BUFFER_SIZE] = { 0 };
	char* ptr = send_msg;
	int len = 0;
	memcpy(ptr, &header, sizeof(Header));
	len += sizeof(Header);
	ptr += sizeof(Header);
	memcpy(ptr, msg.c_str(), msg.size());
	len += msg.size();

	bufferevent_write(bev, send_msg, len);
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