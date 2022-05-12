#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <thread>
#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <csignal>
#include <exception>
#include <string>

#pragma comment(lib,"ws2_32.lib")

using std::cout;
using std::endl;

int myStrLen(char* str)
{
	int i = 0;
	while (*str != '\0')
	{
		++i;
		++str;
	}
	return i;
}

char* myStrcat(char* str1, char* str2)
{
	char* temp = str1;
	while (*temp != '\0')
		++temp;
	while ((*temp++ = *str2++) != '\0');
	return str1;
}

char* myStrcp(char* str1, char* str2)
{
	char* temp = str1;
	while ((*temp++ = *str2++) != '\0');
	return str1;
}

bool myStrSame(char* str1, char* str2)
{
	if (myStrLen(str1) != myStrLen(str2))
		return false;
	for (size_t i = 0; i < myStrLen(str1); ++i)
		if (*str1++ != *str2++)
			return false;
	return true;
}

void myStrSplit(char* str1, char* str2, char* str3, char sp)
//参数:姓名,需要分解信息,得到信息,分隔符
{
	char* temp = str1;
	int i = 0;
	while ((*str3++ = *str2++) != sp)i++;
	while ((*temp++ = *str2++) != '\0');
	*(--str3) = '\0';
}

//start set global variable
SOCKET ServerSocket = INVALID_SOCKET;//服务器套接字
SOCKET ClientSocket = INVALID_SOCKET;//客户端套接字
SOCKADDR_IN ServerAddr = { 0 };//服务器地址
SOCKADDR_IN ClientAddr = { 0 };//客户端地址
USHORT uPort = 10001;//服务器监听端口
int iClientAddrLen = sizeof(ClientAddr);

struct Client
{
	SOCKET client_socket;
	char client_name[255];
};
Client temp_client;
std::vector<Client> Clients;

char buffer[4096] = { 0 };
int iRecvLen = 0;
int iSendLen = 0;
char msg_type = '#';
//end set global varible
//msg type
//# name
//$ show online user
//info@xx send info to xx
//& is exit

bool showClients()
//查看在线用户
{
	if (Clients.size() == 0)
	{
		cout << "online users is NULL\n";
		return false;
	}
	cout << "Online user are: ";
	for (auto it = Clients.begin(); it < Clients.end(); ++it)
		cout << it->client_name <<" ";
	return true;
}

void deleteClient(char* name)
//离开的客户端进行处理
{
	for (auto it = Clients.begin(); it < Clients.end(); ++it)
	{
		if (myStrSame(name, it->client_name))
		{
			Clients.erase(it);
			cout << "Delte client " << name << " success\n";
			return;
		}
	}
	//cout<<"Delete client fail\n";
}

int startServer()
//打开服务器
{
	//存放套接字信息的结构
	WSADATA wsaData = { 0 };
	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("WSAStartup failed with error code: %d\n", WSAGetLastError());
		return -1;
	}
	//判断版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("wVersion was not 2.2\n");
		return -1;
	}
	//创建套接字
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET)
	{
		printf("socket falied with error code: %d\n", WSAGetLastError());
		return -1;
	}

	//设置服务器地址
	ServerAddr.sin_family = AF_INET;//连接方式
	ServerAddr.sin_port = htons(uPort);//服务器监听端口
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//任何客户端都能连接这个服务器

	//绑定服务器
	if (SOCKET_ERROR == ::bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		printf("bind failed with error code: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		return -1;
	}
	if (SOCKET_ERROR == listen(ServerSocket, 20))
	{
		printf("listen failed with error code: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return -1;
	}
	cout << "Starting server...\n";
	return 0;
}

bool isExistClient(char* name)
//判断客户端是否存在Clients中
{
	for (auto it = Clients.begin(); it < Clients.end(); ++it)
	{
		if (myStrSame(name, it->client_name))return true;
	}
	return false;
}

void sendMessage(Client* client, char* message, char* name)
//发送消息，目的地的name，以及消息message，转发的消息格式：message+@+sender name
{
	char buf[2048] = { 0 };
	buf[0] = '\0';
	myStrcat(buf, message);
	char temp[10] = "@";
	myStrcat(buf, temp);
	myStrcat(buf, client->client_name);
	int ret = 0;
	//寻找目的客户端，开始发送信息
	for (auto it = Clients.begin(); it < Clients.end(); ++it)
	{
		if (myStrSame(name, it->client_name))
		{
			ret = send(it->client_socket, buf, myStrLen(buf), 0);
			break;
		}
	}
	if (SOCKET_ERROR == ret)
	{
		printf("send failed with error code: %d\n", WSAGetLastError());
		deleteClient(client->client_name);
		WSACleanup();
	}
	else
	{
		cout << client->client_name << " to " << name << " send: " << buf << endl;
		buf[0] = '\0';
	}
}

void recvMessage(Client* client)
//读取客户端信息
{
	char buf[1024] = { 0 };
	int ret = 0;
	memset(buf, 0, sizeof(buf));
	ret = recv(client->client_socket, buf, sizeof(buf), 0);

	if (SOCKET_ERROR == ret)
	{
		deleteClient(client->client_name);
		return;
	}
	else
	{
		char judge_type = buf[myStrLen(buf) - 1];
		buffer[myStrLen(buf) - 1] = '\0';
		switch (judge_type)
		{
		//将客户端加入到Clients，选择忽略，在别的函数已经实现
		case '#':
		{
			break;
		}
		//减少clients
		case '&':
		{
			deleteClient(client->client_name);
			break;
		}
		case '$':
		{
			showClients();
			break;
		}
		default:
		{
			//client message+@+dest_name
			char* message = (char *)malloc(2048);
			char* dest_name = (char *)malloc(35);
			//参数：姓名，分解信息，得到信息，分隔符
			myStrSplit(dest_name, buf, message, '@');
			sendMessage(client, message, dest_name);
			buf[0] = '\0';
			free(message);
			free(dest_name);
			break;
		}
		}
	}
}

void forwardingMessage()
//接收和转发消息的函数
{
	while (1)
	{
		for (auto it = Clients.begin(); it < Clients.end(); ++it)
		{
			if (isExistClient(it->client_name))
			{
				std::thread t1(recvMessage, &(*it));
				t1.detach();
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::this_thread::yield();
	}
}

void addClient()
//处理得到的client socket的名字，将temp@name替换掉
{
	for (auto it = Clients.begin(); it < Clients.end(); ++it)
	{
		char temp[64] = "temp@name";
		if (myStrSame(it->client_name, temp))
		{
			memset(buffer, 0, sizeof(buffer));
			//接收客户端消息，消息传递进来的名字才接受
			iRecvLen = recv(it->client_socket, buffer, sizeof(buffer), 0);
			char judge_type = buffer[myStrLen(buffer) - 1];
			buffer[myStrLen(buffer) - 1] = '\0';
			//if(isExistClient(buffer)){cout<<"Client exits: "<<buffer<<endl;return;}
			if (judge_type == '#')
			{
				myStrcp(it->client_name, buffer);
				buffer[0] = '\0';
				cout << "添加用户成功\n";
				return;
			}
		}
	}

	cout << "添加用户失败\n";
	return;
}

int main()
{
	//1.打开服务器，让其可接受连接
	startServer();

	//2.读取和转发消息的线程
	std::thread forward_thread(forwardingMessage);
	forward_thread.detach();//并发进程

	//3.开始进行主线程循环
	while (1)
	{
		ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &iClientAddrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			printf("accept failed with error code: %d\n", WSAGetLastError());
			closesocket(ServerSocket);
			WSACleanup();
			continue;
		}
		else
		{
			printf("有客户端接入 IP: %s Port: %d\n\n", inet_ntoa(ClientAddr.sin_addr), htons(ClientAddr.sin_port));
			//将客户端的socket存起来，后面修改name
			Client temp_client;
			char temp[64] = "temp@name";
			myStrcp(temp_client.client_name, temp);
			temp_client.client_socket = ClientSocket;
			Clients.push_back(temp_client);
			addClient();//add user
			showClients();//show online users
		}
		Sleep(2);
	}

	return 0;
}