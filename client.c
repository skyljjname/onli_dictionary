/*************************************************************************
#	 FileName	: server.c
#	 Author		: fengjunhui 
#	 Email		: 18883765905@163.com 
#	 Created	: 2018年12月29日 星期六 13时44分59秒
 ************************************************************************/

#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#define N 128

#define R  1
#define L  2
#define Q  3
#define H  4
#define T  5

struct msg{
	int type;
	char name[32];
	char passwd[32];
	char text[N];
};

void do_query(int sockfd,struct msg msg)
{
	//封装命令请求－发送查询数据，等待接收响应，打印查询结果
	msg.type = Q;

	while(1){
		//数据交互 
		printf("input word (# quit):");
		fgets(msg.text,sizeof(msg.text),stdin);
		msg.text[strlen(msg.text)-1] = '\0';
		if(strcmp(msg.text,"#") == 0)
			break;
		send(sockfd,&msg,sizeof(msg),0);
		recv(sockfd,&msg,sizeof(msg),0);
		printf("**********%s",msg.text);
	}
}

void do_history(int sockfd,struct msg msg)
{
	//封装命令－发送请求－等待响应－打印输出结果
	msg.type = H;
	send(sockfd,&msg,sizeof(msg),0);
	while(1){
		recv(sockfd,&msg,sizeof(msg),0);
		if(msg.text[0] == '\0')
			break;
		printf("%s.\n",msg.text);
	}
}

void do_register(int sockfd,struct msg msg)
{
	//输入用户名－输入密码－封装命令请求－发送请求－等待响应 
	int sendbytes;

	msg.type = R;
	printf("please input your name >>>");
	scanf("%s",msg.name);
	getchar();

	printf("please input your passwd >>>");
	scanf("%s",msg.passwd);
	getchar();
	memset(msg.text,0,sizeof(msg.text));

	send(sockfd,&msg,sizeof(msg),0);
	recv(sockfd,&msg,sizeof(msg),0);
	printf("%s.\n",msg.text);
}

void do_login(int sockfd,struct msg msg)
{
	//输入用户名－输入密码－发送登录请求－接收服务器反馈－
	//　如果反馈为ＯＫ，则继续判断用户需求－跳转对应需求执行
	msg.type = L;
	printf("please input your name >>>");
	scanf("%s",msg.name);
	getchar();

	printf("please input your passwd >>>");
	scanf("%s",msg.passwd);
	getchar();

	send(sockfd,&msg,sizeof(msg),0);
	recv(sockfd,&msg,sizeof(msg),0);

	if(strncmp(msg.text,"OK",2) == 0){
		while(1){
			int choice;

			printf("*********************************.\n");
			printf("**1 query 2 history 3 quit******.\n");
			printf("*********************************.\n");

			printf("please input your choice>>>");
			scanf("%d",&choice);
			getchar();

			switch(choice){
				case 1:
					do_query(sockfd,msg);
					break;
				case 2:
					do_history(sockfd,msg);
					break;
				case 3:
					close(sockfd);
					exit(EXIT_SUCCESS);
			}
		}
	}else{
		printf("%s.\n",msg.text);
	}
}

void do_quit(int sockfd,struct msg msg)
{
	msg.type = T;
	send(sockfd,&msg,sizeof(msg),0);
	recv(sockfd,&msg,sizeof(msg),0);
	if(strncmp(msg.text,"quit",4) == 0)
		printf("thank you for your service.\n");
}

int main(int argc, const char *argv[])
{
	//socket->填充->绑定->监听->等待连接->数据交互->关闭 
	int sockfd;
	int acceptfd;
	ssize_t recvbytes,sendbytes;
	char buf[N] = {0};
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t addrlen = sizeof(serveraddr);
	socklen_t cli_len = sizeof(clientaddr);
	//创建网络通信的套接字
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed.\n");
		exit(-1);
	}
	printf("sockfd :%d.\n",sockfd); 

	//填充网络结构体
	memset(&serveraddr,0,sizeof(serveraddr));
	memset(&clientaddr,0,sizeof(clientaddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port   = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(sockfd,(const struct sockaddr *)&serveraddr,addrlen) == -1){
		perror("connect failed.\n");
		exit(-1);
	}

	int choice;
	struct msg msg;
	while(1){
		printf("*********************************.\n");
		printf("**1 register 2 login 3 quit******.\n");
		printf("*********************************.\n");

		printf("input your choice:");
		scanf("%d",&choice);
		getchar();

		switch(choice){
			case 1:
				do_register(sockfd,msg);
				break;
			case 2:
				do_login(sockfd,msg);
				break;
			case 3:
				do_quit(sockfd,msg);
				break;
		}
#if 0
		//数据交互 
		printf("please input >>>");
		fgets(buf,sizeof(buf),stdin);
		buf[strlen(buf)-1] = '\0';
		if(strncmp(buf,"quit",4) == 0)
			break;
		sendbytes = send(sockfd,buf,sizeof(buf),0);
		if(sendbytes == -1){
			printf("send failed.\n");
			break;
		}
		//send 
		recv(sockfd,buf,sizeof(buf),0);
		printf("buf :%s.\n",buf);
#endif 
	}

	close(sockfd);

	return 0;
}
