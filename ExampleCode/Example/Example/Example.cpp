// Example.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#include <string>
#include<map>
#include<iterator>

//#include <winsock2.h>
//#include <ws2tcpip.h>

//#pragma comment (lib, "Ws2_32.lib")

using namespace std;

class Channel
{
public:
	bufferevent* bev;   /* ÿ��socket��Ӧһ��bufferevent */
	char buf[4096];
	int readSize;
	int packetSize;
	string client_ip;

	Channel()
	{
		readSize = 0;
	}
};

map<string, Channel*> chmap;


//��ȡ����ͷ
void readHeader(struct bufferevent * bev, Channel * ch)
{
	ch->readSize += bufferevent_read(bev, ch->buf + ch->readSize, 4 - ch->readSize);
	if (ch->readSize == 4)   //����ͷ������
	{
		//�����Ч���ݵĴ�С
		ch->packetSize = ntohl(*(uint32_t*)(ch->buf));
	}
	else
	{
		//����ͷû�ж��꣬������һ����ʹbev���ã��ȴ������¶Լ��Ϸ���֪ͨ�ٶ�ȡ
	}
}

//��ȡ������
void readData(struct bufferevent * bev, Channel * ch)
{
	ch->readSize += bufferevent_read(bev, ch->buf + ch->readSize,
		ch->packetSize + 4 - ch->readSize);
}

void dataTransform(struct bufferevent * bev,
	char* content, string ip)
{
	char buf[4096];
	printf(buf, "%s|%s", ip.c_str(), content);
	uint32_t len = strlen(buf);
	len = htonl(len);
	bufferevent_write(bev, (char*)&len, 4);
	bufferevent_write(bev, buf, strlen(buf));
}

void handleData(Channel * ch)
{
	//��������	����192.168.x.x|�ı�
	ch->buf[ch->readSize] = 0;
	char * buf = ch->buf + 4;

	char * ptr;
	char * c_ip = strtok_s(buf, "|", &ptr); //��һ���ͻ��ˣ��������ݣ���ip
	char * content = strtok_s(NULL, "\0", &ptr);//��������

	//�ж���һ���ͻ�������
	Channel * peer = chmap[c_ip];
	if (peer == NULL)
	{
		//����Ϣ�� �����ݵĿͻ��ˣ�֪ͨ���Է�������
		char info[1024];
		printf(info, "ϵͳ��ʾ��%s������", c_ip);
		dataTransform(ch->bev, info, c_ip);
	}
	else
	{
		//ת����Ϣ
		dataTransform(peer->bev, content, ch->client_ip);
	}

	ch->readSize = 0;
	//ch->packetSize = 0;
	//memset(ch->buf, 0, sizeof(ch->buf));
}

// read socketdata
//ĳ���ͻ��˷����ݹ����ˣ���Ҫ������������ݣ�ת������һ���ͻ���
void readcb(struct bufferevent *bev, void *ctx)
{
	//printf("readcb\n");
	Channel * ch = (Channel *)ctx;
	//��ͨ���е����ݶ��������浽���ݽṹ����
	if (ch->readSize < 4)
	{
		readHeader(bev, ch);
		if (ch->readSize == 4)//�����һ�������Ƿ������ͷ������
		{
			//��������
			readData(bev, ch);
		}
		else
		{
			//������һ���жϣ���bev���ã��ȴ������¶Լ��Ϸ���֪ͨ�ٶ�ȡ
		}
	}
	else
	{
		//��������
		readData(bev, ch);
	}

	//�ж������Ƿ�ȫ������
	//����bev���ã��ȴ������¶Լ��Ϸ���֪ͨ�ٶ�ȡ
	if (ch->readSize == ch->packetSize + 4)
	{
		handleData(ch);
	}
	else
	{

	}
}

// write socket
void writecb(struct bufferevent *bev, void *ctx)
{
}

//˵���д���������Ҫclose socket
void eventcb(struct bufferevent *bev, short what, void *ctx)
{
	Channel * ch = (Channel *)ctx;
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "connect �������" << endl;
	}
	else
	{
		if (what & BEV_EVENT_EOF)
			cout << "�ͻ��ˣ�" << ch->client_ip << " �ر���socket" << endl;
		bufferevent_free(bev);
		chmap.erase(ch->client_ip);
	}
}

//���û�����֪ͨ����
void newUserOnline(Channel * ch)
{
	//����map�������û�ip�������û��������û�ip�������û�
	map<string, Channel*>::iterator it;
	for (it = chmap.begin(); it != chmap.end(); ++it)
	{
		Channel * olduser = it->second;

		//���෢��ip------����ip
		const char * olduserip = olduser->client_ip.c_str();
		const char * newuserip = ch->client_ip.c_str();
		uint32_t len = 0;

		//printf("���û�%s\n���û�%s\n", olduserip, newuserip);

		len = strlen(olduserip);//���û�ip����
		len = htonl(len);
		bufferevent_write(ch->bev, (char*)&len, 4);//ip���ȷ������û�
		bufferevent_write(ch->bev, olduserip, strlen(olduserip));//ip�������û�

		len = strlen(newuserip);//���û�ip����
		len = htonl(len);
		bufferevent_write(olduser->bev, (char*)&len, 4);//���û�ip���ȷ������û�
		bufferevent_write(olduser->bev, newuserip, strlen(newuserip));//���û�ip�������û�
	}
}

// listen_bind�Ļص�����
void listen_cb(struct evconnlistener* listener, /*������*/
	evutil_socket_t newfd,   /*���ӿͻ��˵�socket, accept���ص�socket*/
	struct sockaddr* addr,  /*�ͻ��˵�ַ*/
	int socklen,                     /*�ͻ��˵�ַ����*/
	void* ptr)
{
	// ===========ȡ���ͻ��˵�ip��ַ
	struct sockaddr_in * client_addr = (struct sockaddr_in *)addr;
	// ===========������ip��ַת���ַ�����ʽ��ip��ַ
	char sendBuf[20] = { '\0' };
	inet_ntop(AF_INET, (void*)&client_addr->sin_addr, sendBuf, 16);
	string cli_addr = sendBuf;
	printf("�����ӵĿͻ���ip��%s\n", sendBuf);

	if (evutil_make_socket_nonblocking(newfd) < 0)
	{
		perror("failed to set client socket to non-blocking");
		return;
	}

	// ��ȡbase����
	struct event_base* base = (struct event_base*)ptr;
	//struct event_base* base = evconnlistener_get_base(listener);//����2

	// ��newfd����һ��bev
	// ��bev���뵽base������
	struct bufferevent* bev = bufferevent_socket_new(base,
		newfd, BEV_OPT_CLOSE_ON_FREE);

	Channel * ch = new Channel;
	// ����bev�¼��Ļص�����
	bufferevent_setcb(bev, readcb, NULL, eventcb, ch/*�ص���������*/);

	// ����δ��״̬,��bevȥ����ָ������Ϣ
	// �������Ϣ����ص�readcb, writecb, eventcb
	bufferevent_enable(bev, EV_READ | EV_WRITE);//����Ĭ����EV_PERSIST��

	//============�����û�����ʱ������֪ͨȫ�����߳�Ա������û���ip
	//============ͬʱ�ѵ�ǰ���������û���������û���һ��
	//============Ϊ���û�����һ��Channel�ṹ�������
	ch->client_ip = cli_addr;
	ch->bev = bev;

	/*
	//=====��������
	const char * login = "��¼�ɹ�";
	uint32_t len = strlen(login);
	len = htonl(len);
	bufferevent_write(bev, (char*)&len, 4);
	bufferevent_write(bev, login, strlen(login));
	*/

	newUserOnline(ch);

	chmap[cli_addr] = ch;
}

int main()
{
	// Initialize Winsock
	WSADATA wsaData;

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		std::cout << "WSAStartup Failed with error: " << iResult << std::endl;
		return 1;
	}

	cout << "Server begin running!" << endl;
	// ���ƴ���һ��epoll����select����Ĭ����linux��Ӧ��epoll
   // event_base��һ���¼��ļ���
   // ��libevent�У��¼�ָһ������Ҫ�������飬evbase�������������Щ�¼���
   // �����������������
	struct event_base * base = event_base_new();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9988);
	addr.sin_addr.s_addr = INADDR_ANY;

	// ����һ��������--������socket�������뵽base�����У��ü������ǳ�ʼ��״̬�����пͻ�������ʱ���ص�listen_cb����
	// base��addr��ַ������
	//evconnlistener_cb cb = listen_cb;   // �ص�����
	struct evconnlistener* listener = evconnlistener_new_bind(
		base,           // �󶨺ͼ������¼����ϵĶ���
		listen_cb,    // �ص��������������пͻ���������ʱ���ص��������
		base,           // �ص����������һ������
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,  // reuseable:TIME_WAIEʧЧ���쳣�˳��˿ڿ���������      closeonfree��listener���ͷ�ʱ����sockets
		250,            // ���ͬʱ������socket������
		(struct sockaddr*)&addr,    //  ��ַ
		sizeof(addr));
	// ������Ϻ�base�����л���listener
	if (!listener) {
		cout << "Could not create a listener" << endl;
		return 1;
	}

	// ����δ��״̬,ʹlistener������, ֻ��pending״̬��event���ſ��ܱ�����
	evconnlistener_enable(listener);

	// base������������δ��״̬���¼�,������ѭ��,����epoll_wait,���event����ready����ô���event�ͱ�ɼ���״̬��ȥ���ص�������listenerֻ��һ���ص���������ͨsocket���ɵ�bev�������ص�����
	event_base_dispatch(base);

	// ����һ������������˵�������Ҫ�����ˣ����������ͷ��ڴ�ռ�
	event_base_free(base);

	return 0;
}


