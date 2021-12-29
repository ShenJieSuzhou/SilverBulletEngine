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
	bufferevent* bev;   /* 每个socket对应一个bufferevent */
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


//读取数据头
void readHeader(struct bufferevent * bev, Channel * ch)
{
	ch->readSize += bufferevent_read(bev, ch->buf + ch->readSize, 4 - ch->readSize);
	if (ch->readSize == 4)   //数据头读完了
	{
		//获得有效数据的大小
		ch->packetSize = ntohl(*(uint32_t*)(ch->buf));
	}
	else
	{
		//数据头没有读完，返回上一级，使bev重置，等待他重新对集合发出通知再读取
	}
}

//读取数据体
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
	//处理数据	长度192.168.x.x|文本
	ch->buf[ch->readSize] = 0;
	char * buf = ch->buf + 4;

	char * ptr;
	char * c_ip = strtok_s(buf, "|", &ptr); //另一个客户端（接收数据）的ip
	char * content = strtok_s(NULL, "\0", &ptr);//数据内容

	//判断另一个客户端在线
	Channel * peer = chmap[c_ip];
	if (peer == NULL)
	{
		//发消息给 发数据的客户端，通知他对方已下线
		char info[1024];
		printf(info, "系统提示：%s不在线", c_ip);
		dataTransform(ch->bev, info, c_ip);
	}
	else
	{
		//转发消息
		dataTransform(peer->bev, content, ch->client_ip);
	}

	ch->readSize = 0;
	//ch->packetSize = 0;
	//memset(ch->buf, 0, sizeof(ch->buf));
}

// read socketdata
//某个客户端发数据过来了，需要接受里面的数据，转发给另一个客户端
void readcb(struct bufferevent *bev, void *ctx)
{
	//printf("readcb\n");
	Channel * ch = (Channel *)ctx;
	//将通道中的数据读出来保存到数据结构体中
	if (ch->readSize < 4)
	{
		readHeader(bev, ch);
		if (ch->readSize == 4)//检查上一个函数是否把数据头读完了
		{
			//读数据体
			readData(bev, ch);
		}
		else
		{
			//进入下一个判断，把bev重置，等待他重新对集合发出通知再读取
		}
	}
	else
	{
		//读数据体
		readData(bev, ch);
	}

	//判断数据是否全部读完
	//否则将bev重置，等待他重新对集合发出通知再读取
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

//说明有错误发生，需要close socket
void eventcb(struct bufferevent *bev, short what, void *ctx)
{
	Channel * ch = (Channel *)ctx;
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "connect 操作完成" << endl;
	}
	else
	{
		if (what & BEV_EVENT_EOF)
			cout << "客户端：" << ch->client_ip << " 关闭了socket" << endl;
		bufferevent_free(bev);
		chmap.erase(ch->client_ip);
	}
}

//新用户上线通知工作
void newUserOnline(Channel * ch)
{
	//遍历map，把老用户ip发给新用户，把新用户ip发给老用户
	map<string, Channel*>::iterator it;
	for (it = chmap.begin(); it != chmap.end(); ++it)
	{
		Channel * olduser = it->second;

		//互相发送ip------长度ip
		const char * olduserip = olduser->client_ip.c_str();
		const char * newuserip = ch->client_ip.c_str();
		uint32_t len = 0;

		//printf("老用户%s\n新用户%s\n", olduserip, newuserip);

		len = strlen(olduserip);//老用户ip长度
		len = htonl(len);
		bufferevent_write(ch->bev, (char*)&len, 4);//ip长度发给新用户
		bufferevent_write(ch->bev, olduserip, strlen(olduserip));//ip发给新用户

		len = strlen(newuserip);//新用户ip长度
		len = htonl(len);
		bufferevent_write(olduser->bev, (char*)&len, 4);//新用户ip长度发给老用户
		bufferevent_write(olduser->bev, newuserip, strlen(newuserip));//新用户ip发给老用户
	}
}

// listen_bind的回调函数
void listen_cb(struct evconnlistener* listener, /*监听器*/
	evutil_socket_t newfd,   /*连接客户端的socket, accept返回的socket*/
	struct sockaddr* addr,  /*客户端地址*/
	int socklen,                     /*客户端地址长度*/
	void* ptr)
{
	// ===========取出客户端的ip地址
	struct sockaddr_in * client_addr = (struct sockaddr_in *)addr;
	// ===========将整型ip地址转成字符串格式的ip地址
	char sendBuf[20] = { '\0' };
	inet_ntop(AF_INET, (void*)&client_addr->sin_addr, sendBuf, 16);
	string cli_addr = sendBuf;
	printf("新连接的客户端ip是%s\n", sendBuf);

	if (evutil_make_socket_nonblocking(newfd) < 0)
	{
		perror("failed to set client socket to non-blocking");
		return;
	}

	// 获取base集合
	struct event_base* base = (struct event_base*)ptr;
	//struct event_base* base = evconnlistener_get_base(listener);//方法2

	// 用newfd创建一个bev
	// 把bev放入到base集合中
	struct bufferevent* bev = bufferevent_socket_new(base,
		newfd, BEV_OPT_CLOSE_ON_FREE);

	Channel * ch = new Channel;
	// 设置bev事件的回调函数
	bufferevent_setcb(bev, readcb, NULL, eventcb, ch/*回调函数参数*/);

	// 进入未决状态,让bev去监听指定的消息
	// 如果有消息，会回调readcb, writecb, eventcb
	bufferevent_enable(bev, EV_READ | EV_WRITE);//可能默认是EV_PERSIST的

	//============当新用户连接时，首先通知全体在线成员这个新用户的ip
	//============同时把当前所有在线用户给这个新用户发一遍
	//============为新用户创建一个Channel结构体存数据
	ch->client_ip = cli_addr;
	ch->bev = bev;

	/*
	//=====测试数据
	const char * login = "登录成功";
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
	// 类似创建一个epoll或者select对象，默认下linux下应该epoll
   // event_base是一个事件的集合
   // 在libevent中，事件指一件即将要发生事情，evbase就是用来监控这些事件的
   // 发生（激活）的条件的
	struct event_base * base = event_base_new();

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9988);
	addr.sin_addr.s_addr = INADDR_ANY;

	// 创建一个监听器--服务器socket，并加入到base集合中，该监听器是初始化状态，当有客户端连接时，回调listen_cb函数
	// base绑定addr地址并监听
	//evconnlistener_cb cb = listen_cb;   // 回调函数
	struct evconnlistener* listener = evconnlistener_new_bind(
		base,           // 绑定和监听的事件集合的对象
		listen_cb,    // 回调函数，当发现有客户端来连接时，回调这个函数
		base,           // 回调函数的最后一个参数
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,  // reuseable:TIME_WAIE失效，异常退出端口可立即复用      closeonfree：listener被释放时清理sockets
		250,            // 最大同时监听的socket的数量
		(struct sockaddr*)&addr,    //  地址
		sizeof(addr));
	// 创建完毕后，base集合中会有listener
	if (!listener) {
		cout << "Could not create a listener" << endl;
		return 1;
	}

	// 进入未决状态,使listener被启动, 只有pending状态的event，才可能被激活
	evconnlistener_enable(listener);

	// base监听它的所有未决状态的事件,进入死循环,类似epoll_wait,如果event条件ready，那么这个event就变成激活状态，去调回调函数，listener只有一个回调函数，普通socket生成的bev有三个回调函数
	event_base_dispatch(base);

	// 从上一个函数出来后，说明程序快要结束了，这里用来释放内存空间
	event_base_free(base);

	return 0;
}


