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
#include <time.h>
#include <sqlite3.h>
#include <signal.h>

#define N 128
#define DATABASE "database.db"

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while(0)

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

void get_system_time(char* timedata)
{
	time_t t;
	struct tm *tp;

	time(&t);
	tp = localtime(&t);
	sprintf(timedata,"%d-%d-%d %d:%d:%d",tp->tm_year+1900,tp->tm_mon+1,\
			tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
	return ;
}

void process_register_request(int acceptfd,struct msg msg,sqlite3 *db)
{
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	//封装sql命令－插入数据库－如果插入成功则发送ＯＫ通知客户端
	char sql[N] = {0};
	char timedata[N] = {0};
	char *errmsg;

	sprintf(sql,"insert into usrinfo values('%s','%s');",msg.name,msg.passwd);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
		strcpy(msg.text,"failed");
		send(acceptfd,&msg,sizeof(msg),0);
		return ;
	}else{
		//系统管理员整体的查询
		//get_system_time(timedata);
		//	sprintf(sql,"insert into historyinfo values('%s','%s','%s')",timedata,msg.name,"register");
		strcpy(msg.text,"OK");
		send(acceptfd,&msg,sizeof(msg),0);
		printf("%s register success.\n",msg.name);
	}
}

void process_login_request(int acceptfd,struct msg msg,sqlite3 *db)  
{
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	//封装ｓｑｌ命令，表中查询用户名和密码－存在－登录成功－发送响应－失败－发送失败响应	
	char sql[N] = {0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;

	sprintf(sql,"select * from usrinfo where name='%s' and passwd='%s';",msg.name,msg.passwd);
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);		
	}

	if(nrow == 0){
		strcpy(msg.text,"name or passwd failed.\n");
		send(acceptfd,&msg,sizeof(msg),0);
		return;
	}else{
		strcpy(msg.text,"OK");
		send(acceptfd,&msg,sizeof(msg),0);
		return;
	}
}

int do_searchword(int acceptfd,struct msg *msg)
{
	//打开字典－每次读一行－和msg.text比较－ＯＫ－字符串处理－获取之后的结果－send发送数据
	//no-ok 继续查找－到结尾－没有查找到－send　words error　－
	int wordlen;
	FILE *fp;
	int retval;
	char *p;
	char buf[512] = {0};

	fp = fopen("dict.txt","r");
	if(fp == NULL){
		handle_error("fopen failed.\n");
	}
	wordlen = strlen(msg->text);
	printf("query words is %s len = %d.\n",msg->text,strlen(msg->text));

	while(fgets(buf,sizeof(buf),fp) != NULL){
		retval = strncmp(msg->text,buf,wordlen);
		if(retval > 0)
			continue;
		if(retval < 0 || buf[wordlen] != ' ')
			break;

		p = buf + wordlen;
		while(*p == ' ')
			p++;

		strcpy(msg->text,p);
		fclose(fp);
		return 1;
	}
	return 0;
}

void process_query_request(int acceptfd,struct msg msg,sqlite3 *db)  
{
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	int found_flags;
	char sql[N] = {0};
	char words[N] = {0};
	char timedata[N] = {0};
	char *errmsg;

	strcpy(words,msg.text);
	found_flags = do_searchword(acceptfd,&msg);
	if(found_flags){
		get_system_time(timedata);
		sprintf(sql,"insert into historyinfo values('%s','%s','%s');",timedata,msg.name,words);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			printf("%s.\n",errmsg);
		}
	}else{
		strcpy(msg.text,"not found.\n");
	}
	send(acceptfd,&msg, sizeof(msg), 0);

}

//回调函数
	int flags = 0;
int history_callback(void *arg, int ncolumn, char **f_value, char **f_name)
{
	int i = 0;
	int acceptfd = *((int *)arg);
	struct msg msg;

	if(flags == 0)
	{
		for(i = 0; i < ncolumn; i++)
		{
			printf("%-11s", f_name[i]);
		}
		putchar(10);
		flags = 1;
	}

	for(i = 0; i < ncolumn; i+=3)
	{
		sprintf(msg.text,"%s-%s-%s",f_value[i],f_value[i+1],f_value[i+2]);
		send(acceptfd,&msg,sizeof(msg),0);
	}
//	putchar(10);

	return 0;
}



void process_history_request(int acceptfd,struct msg msg,sqlite3 *db)  
{
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	//封装sql命令－查找历史记录表－回调函数－发送查询结果－发送结束标志
	char sql[N] = {0};
	char *errmsg;

	sprintf(sql,"select * from historyinfo where name='%s';",msg.name);
	if(sqlite3_exec(db,sql,history_callback,(void *)&acceptfd,&errmsg) != SQLITE_OK){
		printf("%s.\n",errmsg);		
	}else{
		printf("query record done.\n");
	}

	//通知对方查询结束了
	msg.text[0] = '\0';
	send(acceptfd,&msg,sizeof(msg),0);
}

void process_quit_request(int acceptfd,struct msg msg,sqlite3 *db)  
{
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	strcpy(msg.text,"quit");
	send(acceptfd,&msg,sizeof(msg),0);
	close(acceptfd);
}

int main(int argc, const char *argv[])
{
	//socket->填充->绑定->监听->等待连接->数据交互->关闭 
	int sockfd;
	int acceptfd;
	ssize_t recvbytes;
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

	//绑定网络套接字和网络结构体
	if(bind(sockfd, (const struct sockaddr *)&serveraddr,addrlen) == -1){
		printf("bind failed.\n");
		exit(-1);
	}

	//监听套接字，将主动套接字转化为被动套接字
	if(listen(sockfd,10) == -1){
		printf("listen failed.\n");
		exit(-1);
	}

	char sql[N] = {0};
	char *errmsg;
	sqlite3 *db;

	if(sqlite3_open(DATABASE,&db) != SQLITE_OK){
		printf("%s.\n",sqlite3_errmsg(db));
	}else{
		printf("the database open success.\n");
	}

	if(sqlite3_exec(db,"create table usrinfo(name text primary key,passwd text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}

	if(sqlite3_exec(db,"create table historyinfo(time text,name text,words text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create historyinfo table success.\n");
	}
	
	signal(SIGCHLD, SIG_IGN);

	pid_t pid;
	struct msg msg;

	while(1){
		//数据交互 
		acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&cli_len);
		if(acceptfd == -1){
			printf("acceptfd failed.\n");
			exit(-1);
		}
		pid = fork();
		if(pid < 0){
			handle_error("fork failed.\n");
		}
		if(pid == 0){
			while(1){
				recv(acceptfd,&msg,sizeof(msg),0);
				printf("msg.type :%d.\n",msg.type);
				switch(msg.type)
				{
					case R:
						process_register_request(acceptfd,msg,db);
						break;
					case L:
						process_login_request(acceptfd,msg,db);
						break;
					case Q:
						process_query_request(acceptfd,msg,db);
						break;
					case H:
						process_history_request(acceptfd,msg,db);
						break;
					case T:
						process_quit_request(acceptfd,msg,db);
						break;
				}
			}
		}
		if(pid > 0){
			printf("client ip: %s.\n",inet_ntoa(clientaddr.sin_addr));
			close(acceptfd);
		}
	}

	close(sockfd);

	return 0;
}
