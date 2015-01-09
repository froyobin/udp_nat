//AA
//:q
//
//  main.cpp
//  network_nat
//
//  Created by jiaojiao on 15/1/4.
//  Copyright (c) 2015年 jiaojiao. All rights reserved.
//
#include   <sys/stat.h>
#include <sys/socket.h>
#include <stdio.h>
#include   <sys/types.h>
#include   <string>
#include <cstring>
#include   <sys/socket.h>
#include <pthread.h>
#include   <iostream>
#include <cstdio>
#include <cstdlib>
#include   <netdb.h>
#include <arpa/inet.h>
#include   <fcntl.h>
#include   <unistd.h>
#include   <netinet/in.h>
#include <signal.h>
#include   <arpa/inet.h>
#include <sys/wait.h>
using namespace std;

const unsigned long    RES_LENGTH=10240;//接受字符的最大长度
int    connect_socket(string server,int serverPort);
int     send_msg(int sockfd,string sendBuff,struct sockaddr_in *);
int     send_msg_withflag(int sockfd,string sendBuff,int flags,struct sockaddr_in *);
int recv_msg(int sockfd,char *buf);
void localserver_start(struct sockaddr_in guest);
int recv_msg_with_flag(int sockfd,char *response,int flag);
int     close_socket(int sockfd);
int create_server(struct sockaddr_in guest);
void Stop(int signo);
int start_real_communicate(int sockfd,struct sockaddr_in * client);
const char SERVERALL[]="0.0.0.0";
bool exitflag=false;

struct task_info
{
    int sockfd;
    struct sockaddr_in client;

};

int main(int argc, char ** argv)
{
    int   sockfd=0;
    int   port = 4242;
    string ip;
    bool flag=false;
    if(argc !=4)
    {
        ip=argv[1];
        port = atoi(argv[2]);
        cout<<"Input IP: "<<ip<<", port : "<< port<<endl;
        cout<<"input with ip  port and hostname"<<endl;
        return 0;
    }
    else if(argc > 1)
    {
        port = atoi(argv[1]);
        printf("Input port : %d\n", port);
    }
      sockfd=connect_socket(ip, port);
    if (sockfd<0)
    {
        cout<<"server is not ready"<<endl;
        return 0;
    }

    const string  HOSTNAME=argv[3];

    struct sockaddr_in addr;
    struct hostent *phost;

    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);//按IP初始化

    if(addr.sin_addr.s_addr == INADDR_NONE){//如果输入的是域名
        phost = (struct hostent*)gethostbyname(argv[1]);
        if(phost==NULL){
            herror("Init socket s_addr error!");
            return 0;
        }
        addr.sin_addr.s_addr =((struct in_addr*)phost->h_addr)->s_addr;
    }

    string send_string="0#"+HOSTNAME;
    send_msg(sockfd,send_string,&addr);

    struct sockaddr_in guest;
    socklen_t guest_len = sizeof(guest);
    getsockname(sockfd,(struct sockaddr*)&guest,&guest_len);
    cout<<"behind port"<<ntohs(guest.sin_port)<<endl;

    char *buf =new char[RES_LENGTH];
    bzero(buf,RES_LENGTH);
    recv_msg(sockfd,buf);
    cout<<"server send back:"<<buf<<endl;
    if(atoi(buf)==10) {
        cout<<"I will keep"<<endl;
        signal(SIGINT, Stop);
        pid_t localpid=fork();
        if (localpid!=0) {
            //cin>>input;
            //if(input=="QUIT")
            {
              //  kill(localpid,SIGKILL);
                wait(&localpid);
                string send_string="2#"+HOSTNAME;
                send_msg_withflag(sockfd,send_string,MSG_DONTWAIT,&addr);//addr is the server IP
                close_socket(sockfd);
                return 0;

            }

        }else{
            send_string="1#"+HOSTNAME;
            cout<<send_string<<endl;
            send_msg(sockfd,send_string,&addr);
            struct sockaddr_in client;
            while(true){
            cout<<"client is not ready";
            while(1){
            int ret=recv_msg_with_flag(sockfd,buf,MSG_DONTWAIT);
            if(ret>0){
            memcpy((char *)&client, buf, sizeof(struct sockaddr));
            cout<<ntohs(client.sin_port)<<endl;
            if(ntohs(client.sin_port)!=0)
            {
                break;
            }
            sleep(1);
            if(exitflag==true)
                exit(1);
            }else{
            sleep(1);
            if(exitflag==true)
                exit(1);

            }

            }
            cout<<"client is ready to go"<<endl;
//            wait(&localpid);
//            cout<we exit"<<endl;
            int ret=start_real_communicate(sockfd,&client);
            if(ret==2){
            close_socket(sockfd);
            exit(1);
            }
        }
       }

    }

    printf("return from recv function\n");
    close_socket(sockfd);
    return 0;
}


static void* period_task(void * args)
{
    srand(time(NULL));
    struct task_info *taskinfo=(struct task_info *)args;

    int sockfd=taskinfo->sockfd;
    while(1)
    {
        string msg="KEEPALIVE\n";   //if we donot use \n, it will store the data in read buff,and data is not show up instantly
        send_msg_withflag(sockfd,msg,MSG_DONTWAIT,&taskinfo->client);
        sleep(rand()%20);
    }

}

int start_real_communicate(int sockfd,struct sockaddr_in * client)
{
    char *readbuf = new char[RES_LENGTH];
    char *writebuf= new char[RES_LENGTH];
    //tchar writebuf[512];
    send_msg_withflag(sockfd,"PROBE",MSG_DONTWAIT,client);
    send_msg_withflag(sockfd,"PROBE",MSG_DONTWAIT,client);

    int exit_reason=0;
    char *buf =new char[RES_LENGTH];
    recv_msg(sockfd,buf);
    cout<<"hole has be digged!!!"<<endl;


    fd_set myfds;
    int sizer;
    int sizew;
    pthread_t mythread;

    struct task_info task;
    task.sockfd = sockfd;
    memcpy(&task.client,client,sizeof(struct sockaddr_in));

    int ret=pthread_create(&mythread,NULL,period_task,(void *)&task);


    while(exit_reason==0)
    {
        bzero(readbuf,RES_LENGTH);
        bzero(writebuf,RES_LENGTH);
        FD_ZERO(&myfds);
        FD_SET(sockfd,&myfds);
        FD_SET(0,&myfds);
        select(sockfd+1,&myfds,NULL,NULL,NULL);
        if(FD_ISSET(sockfd,&myfds))
        {
             sizew = recv_msg(sockfd,writebuf);
             if(sizew>0){
             cout<<writebuf;
             }
             if(strncmp(writebuf,"QUIT",4)==0)
             {
                 exit_reason=1;
             }
        }
        if(FD_ISSET(0,&myfds)){
        ssize_t size = read(0,readbuf,1024);
        string msg=string(readbuf,size);
        if(strncmp(readbuf,"QUIT",4)==0)
        {
        send_msg(sockfd,msg,client);
        exit_reason=2;
        break;

        }
        send_msg(sockfd,msg,client);
        }
    }
        cout<<exit_reason<<endl;
        delete []readbuf;
        delete []writebuf;
        delete []buf;
        return exit_reason;
}
void Stop(int signo)
{
    exitflag=true;
    cout<<"USE QUIT to QUIT"<<endl;

}




void die(string line)
{
    cout<<line<<endl;
}

int create_server(struct sockaddr_in guest)
{
    int yes=-1;
    struct addrinfo *ai;


    int serversockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (serversockfd==-1)
    {
        die("socket can not be created!!");
        return -1;
    }
/*
    if (setsockopt(serversockfd, SOL_SOCKET, SO_REUSEPORT, (const void *)&yes, sizeof(int)) == -1) {
        die("Couldn't setsockopt");
        return -1;
    }
    */
    char portnumber[20];
    sprintf(portnumber, "%d",ntohs(guest.sin_port));
    /* Fill the address info struct (host + port) -- getaddrinfo(3) */
    if (getaddrinfo(SERVERALL,portnumber, NULL, &ai) != 0) {   // get localhost
        cout<<"port number"<<"######"<<portnumber<<endl;
        die("Couldn't get address");
        return -1;
    }
/*
    if(bind(serversockfd, ai->ai_addr)!=0) //mac os use this bind   fixme
    {

        die("Couldn't bind socket to address");
        return -1;
    }
    */
    //    if (bind(serversockfd, ai->ai_addr, ai->ai_addrlen) != 0) {  // only bind on localhost ip
    //        die("Couldn't bind socket to address");
    //        return -1;
    //    }

    /* Free the memory used by our address info struct */
    /* Mark this socket as able to accept incoming connections */
    /* printf("Process %d Listening/n", getpid()); */
    cout<<"creat server at"<<portnumber<<"successfully"<<endl;
    freeaddrinfo(ai);
    //    close(serversockfd);
    //    exit(1);
    return serversockfd;
}
/************************************************************
 * 连接SOCKET服务器，如果出错返回-1，否则返回socket处理代码
 * server：服务器地址(域名或者IP),serverport：端口
 * ********************************************************/
int connect_socket(string server,int serverPort){
    int    sockfd=0;
    int yes=-1;

    //向系统注册，通知系统建立一个通信端口
    //AF_INET表示使用IPv4协议
    //SOCK_STREAM表示使用TCP协议
    if((sockfd=socket(AF_INET,SOCK_DGRAM,0))<0){
        herror("Init socket error!");
        return -1;
    }
/*
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void *)&yes, sizeof(int))==-1)
    {
        herror("cant resuse the port and address");
        return -1;
    }
    */
    return sockfd;
}
/**************************************************************
 * 发送消息，如果出错返回-1，否则返回发送的字符长度
 * sockfd：socket标识，sendBuff：发送的字符串
 * *********************************************************/
int send_msg_withflag(int sockfd,string sendBuff,int flags,struct sockaddr_in *server)
{
    int sendSize=0;
    if((sendSize=sendto(sockfd,sendBuff.c_str(),sendBuff.length(),flags,(struct sockaddr *)server,sizeof(struct sockaddr_in)))<=0){
        herror("Send msg error!");
        return -1;
    }else
        return sendSize;
}
int send_msg(int sockfd,string sendBuff,struct sockaddr_in *server)
{
    int sendSize=0;
    if((sendSize=sendto(sockfd,sendBuff.c_str(),sendBuff.length(),0,(struct sockaddr *)server,sizeof(struct sockaddr_in)))<=0){
        herror("Send msg error!");
        return -1;
    }else
        return sendSize;
}
/****************************************************************
 *接受消息，如果出错返回NULL，否则返回接受字符串的指针(动态分配，注意释放)
 *sockfd：socket标识
 * *********************************************************/
int recv_msg(int sockfd,char *response){
    int  recLenth=0;
    //memset(response,0,RES_LENGTH);


    if(( recLenth=recv(sockfd,response,RES_LENGTH,0))==-1 )
    {
        cout<<response<<endl;
        printf("Return value : %d\n", recLenth);
        perror("Recv msg error : ");
        return -1;
    }

    return 1;
}
int recv_msg_with_flag(int sockfd,char *response,int flag){
    int  recLenth=0;
    memset(response,0,RES_LENGTH);


    if(( recLenth=recv(sockfd,response,RES_LENGTH,flag))==-1 )
    {
        cout<<response<<endl;
     //   printf("Return value : %d\n", recLenth);
      //  perror("Recv msg error : ");
        return -1;
    }

    return recLenth;
}
/**************************************************
 *关闭连接
 * **********************************************/
int close_socket(int sockfd)
{
    close(sockfd);
    return 0;
}
