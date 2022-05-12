#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
using std::cout;
using std::endl;
#pragma comment(lib,"ws2_32.lib")

//字符串处理的函数
int myStrLen(char* str)
{
	int i = 0;
	while (*str != '\0')
	{
		i++;
		str++;
	}
	return i;
}
char* myStrcat(char* str1, char* str2)
{
	char* temp = str1;
	while (*temp != '\0')temp++;
	while ((*temp++ = *str2++) != '\0');
	return str1;
}
char* myStrcp(char* str1, char* str2)
{
	char* temp = str1;
	int i = 0;
	while ((*temp++ = *str2++) != '\0');
	return str1;

}
char* myStrSplit(char* str1, char* str2, char* str3, char sp)
//参数:姓名,需要分解信息,得到信息,分隔符
{
	char* temp = str1;
	int i = 0;
	while ((*str3++ = *str2++) != sp)i++;
	while ((*temp++ = *str2++) != '\0');
	*(--str3) = '\0';
	return str3;
}

//start set global varibal
WSADATA wsaData = { 0 };//存放套接字信息
SOCKET ClientSocket = INVALID_SOCKET;//客户端套接字
SOCKADDR_IN ServerAddr = { 0 };//服务器地址
USHORT uPort = 10001;//服务器端口
char IP[32] = "127.0.0.1";

char msg_type = '#';
char buffer[4096] = { 0 };
char name[30] = { 0 };
int iRecvLen = 0;
int iSendLen = 0;

//end set global varibal
//msg type 
//# name
//@ show online user
//info&xx send info to xx
//& is exit

bool startConnectServer()
//连接服务器
{
	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		printf("WSAStartup failed with error code: %d\n", WSAGetLastError());
		return false;
	}
	//判断套接字版本
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("wVersion was not 2.2\n");
		return false;
	}
	//创建套接字
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("socket failed with error code: %d\n", WSAGetLastError());
		return false;
	}
	//设置服务器地址
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(uPort);//服务器端口
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);//服务器地址
	//连接服务器
	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		printf("Connect failed with error code: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return false;
	}
	printf("连接服务器成功 IP:%s Port: %dn\n\n\n\n", inet_ntoa(ServerAddr.sin_addr), htons(ServerAddr.sin_port));
	return true;
}

void sendMessage(char* buf)
{
	iSendLen = send(ClientSocket, buf, myStrLen(buf), 0);
	if (SOCKET_ERROR == iSendLen)
	{
		printf("Send failed with error code: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
	}
	else
	{
		//<<buf<<endl;
	}
}

void recvMessage()
{
	char buf[1024] = { 0 };
	int ret = 0;
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::this_thread::yield();
		memset(buf, 0, sizeof(buf));
		ret = recv(ClientSocket, buf, sizeof(buf), 0);
		if (SOCKET_ERROR == ret)
		{
			printf("recv failed with error code: %d\n", WSAGetLastError());
			continue;
		}
		else
		{
			char chat_name[30] = { 0 };
			char message[2048] = { 0 };
			myStrSplit(chat_name, buf, message, '@');
			cout << chat_name << ":\n" << message << '\n';
		}
	}
}

void main_thread()
//主线程，用于连接服务器+发送消息
{
	while (1)
	{
		Sleep(10);
		if (startConnectServer())break;
	}
	//输入姓名
	cout << "Please input you name: \n";
	std::cin >> name;
	char temp[10] = "#";
	myStrcat(name, temp);
	sendMessage(name);
	cout << "Now you can chat with others\nhelp:& is exit\n";
	while (1)
	{
		buffer[0] = '\0';
		std::cin >> buffer;
		sendMessage(buffer);
		msg_type = buffer[myStrLen(buffer) - 1];
		Sleep(10);
		if (msg_type == '&')exit(0);//退出客户端，但是前提时发送消息给server，告知退出
	}
}
//
//void normal_thread()
////普通的线程用于读取消息
//{
//	recvMessage();
//}

int main()
{
	std::thread normal_thread(recvMessage);
	//std::thread normal_thread();
	normal_thread.detach();
	main_thread();
	system("pause");
	return 0;
}