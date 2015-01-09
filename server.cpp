#include <unistd.h> /* fork, close */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h> /* exit */
#include <algorithm>
#include <string.h> /* strlen */
#include <string>
#include <vector>
#include <stdio.h> /* perror, fdopen, fgets */
#include <sys/socket.h>
#include <sys/wait.h> /* waitpid */
#include <netdb.h> /* getaddrinfo */
using namespace std;
#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)
const  char PORT[]="42422";
const int  NUM_CHILDREN=3;
const char TARGETADDR[]="0.0.0.0";
const int MAXLEN=1024;
struct client_info
{
struct sockaddr_in client;
string name;
unsigned short int src_port;
string src_addr;
bool operator==(const string c)
{
    if(this->name==c)
        return true;
    else
        return false;
}
};
int readline(int fd, char *buf, int maxlen); // forward declaration
void handle_event(int cliendtd,struct sockaddr_in *client);
void say_hello(int clientfd,struct sockaddr_in *client);
void save_client(int clientfd,struct sockaddr_in*,char *lines);
void handle_hello(struct sockaddr_in *client,int cliendtd,char * lines);
void error_hello(struct sockaddr_in * client,int clientfd,char * lines);
void lsendmsg(int fd,string buff,struct sockaddr_in *client);
void say_bye(char *);
int recvdata(int fd, char *buf, int maxlen,struct sockaddr_in*cliet);
void send_info(int clientfd,struct sockaddr_in *);
void lsendmsgraw(int fd,char *buff,size_t buflength,struct sockaddr_in *client);

vector<struct client_info> client_pool;




int main(int argc, char** argv)
{
	int n, sockfd, clientfd;
	int yes = 1;    // used in setsockopt(2)
	struct addrinfo *ai;
	struct sockaddr_in *client;
	socklen_t client_t;
	//pid_t cpid;     // child pid
	//char line[MAXLEN];
	//optchar cpid_s[32];
	//char welcome[32];
	/* Create a socket and get its file descriptor -- socket(2) */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		die("Couldn't create a socket");
	}
	/* Prevents those dreaded "Address already in use" errors */
	//lib64if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&yes, sizeof(int)) == -1) {
//		die("Couldn't setsockopt");
//	}
    /* Fill the address info struct (host + port) -- getaddrinfo(3) */
    if (getaddrinfo(TARGETADDR, PORT, NULL, &ai) != 0) {   // get localhost
        die("Couldn't get address");
    }
    /* Assign address to this socket's fd */
    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) != 0) {  // only bind on localhost ip
        die("Couldn't bind socket to address");
    }
    /* Free the memory used by our address info struct */
    freeaddrinfo(ai);
    /* Mark this socket as able to accept incoming connections */
    /* printf("Process %d Listening/n", getpid()); */
    while(1)
    {
        char *lines = new char[MAXLEN];
        bzero(lines,sizeof(char)*MAXLEN);
        client=new struct sockaddr_in;
        /* Necessary initialization for accept(2) */
        client_t = sizeof(struct sockaddr_in);
        /* Blocks! */
        printf("Waiting new connection!\n");

        n = recvfrom(sockfd, lines, MAXLEN,0,(struct sockaddr *)client,&client_t);
        lines[n]='\n';
        cout<<"#####################";
        cout<<lines<<"################"<<endl;
        switch(atoi(lines))
        {
            case 0:save_client(sockfd,client,lines);break;
            case 1:send_info(sockfd,client);break;
            case 2:say_bye(lines);break;
            default:say_bye(lines);break;
        }
        delete lines;
        delete client;
    }
    close(sockfd);
    delete client;
    printf("Close server socket./n");
    return 0;
}


void send_info(int clientfd,struct sockaddr_in *cclient)
{
	struct addrinfo *ai;
    if(client_pool.size()<2)
    {
    if (getaddrinfo(TARGETADDR,"0", NULL, &ai) != 0) {   // get localhost
        die("Couldn't get address");
    }
        cout<<"client or server is not ready"<<endl;
        lsendmsgraw(clientfd, (char *)ai->ai_addr ,sizeof(struct sockaddr_in),cclient);
        freeaddrinfo(ai);
        return;
    }
    int i=0;
    for (i=0;i<2;i++){
        char *buff=new char[sizeof(struct sockaddr_in)];
        memcpy(buff,&client_pool[i].client,sizeof(struct sockaddr_in));
        cout<<"#################"<<ntohs(client_pool[i].client.sin_port)<<"####SSSSSEEEEEEENNNNNNDDDDDD#";
        lsendmsgraw(clientfd, buff,sizeof(struct sockaddr_in),&client_pool[(i+1)%2].client);
        delete [] buff;
    }
}
void save_client(int clientfd,struct sockaddr_in *client,char *lines)
{
    struct client_info my_client;
    string data=string(lines);
    my_client.src_port = ntohs(client->sin_port);
    my_client.src_addr=string(inet_ntoa(client->sin_addr));
    unsigned long pos =data.find('#');
    if(pos==data.npos)
    {
        cout<<"error client with wrong data format"<<endl;
    }
    else{
        char *pos_p=lines+pos+1;  //we skip ‘#’
        my_client.name = string(pos_p,strlen(pos_p));
        memcpy(&my_client.client,client,sizeof(struct sockaddr_in));
        cout<<my_client.src_addr<<":"<<(my_client.src_port)<<endl;
        client_pool.push_back(my_client);
    }
    lsendmsg(clientfd, "10", client);
}

void handle_event(int clientfd,struct sockaddr_in *client)
{
    char *buf = new char[MAXLEN];
    say_hello(clientfd,client);
    bool flag=true;
    while(flag)
    {
        cout<<"wait ...."<<endl;
        int length=recvdata(clientfd, buf,MAXLEN,client);
        cout<<buf<<"length:"<<length<<endl;
        switch(atoi(buf))
        {
            case 10: break;
           // case 11: say_bye(clientfd,client);flag=false;break;
            //default:say_bye(clientfd,client);flag=false;break;
        }
    }
    //close(clientfd);  //client will close it
    delete [] buf;

}

void lsendmsg(int fd,string buff,struct sockaddr_in *client)
{
    sendto(fd,buff.c_str(),buff.length(),0,(struct sockaddr *)client,sizeof(struct sockaddr));
}
void lsendmsgraw(int fd,char *buff,size_t buflength,struct sockaddr_in *client)
{
    sendto(fd,buff,buflength,0,(struct sockaddr *)client,sizeof(struct sockaddr));
}

void say_bye(char *lines)
{
    string data=string(lines);
    string name="";
    unsigned long pos =data.find('#');
    if(pos==data.npos)
    {
        cout<<"error client with wrong data format"<<endl;
    }
    else{
        char *pos_p=lines+pos+1;  //we skip ‘#’
        name = string(pos_p,strlen(pos_p));

    }
    cout<<"delete "<<name<<endl;
    client_pool.erase(remove(client_pool.begin(),client_pool.end(),name),client_pool.end());

    //FIXME
    //client_pool.erase(remove(client_pool.begin(),client_pool.end(),*client),client_pool.end());
 //   lsendmsg(clientfd, "11",client);
}
void say_hello(int clientfd,struct sockaddr_in *client)
{
    lsendmsg(clientfd, "10",client);
}
void error_hello(struct sockaddr_in * client,int clientfd,char * lines)
{
    struct client_info my_client;
    my_client.name=lines;
    my_client.name="UNKNOW";
    lsendmsg(clientfd, "NAMEERROR", client);
    delete client;
    close(clientfd);
}
/*
 * Simple utility function that reads a line from a file descriptor fd,
 * up to maxlen bytes -- ripped from Unix Network Programming, Stevens.
 */
int readline(int fd, char *buf, int maxlen)
{
    int n, rc;
    char c;
    for (n = 1; n < maxlen; n++) {
        if ((rc = read(fd, &c, 1)) == 1) {
            *buf++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0; // EOF, no data read
            else
                break; // EOF, read some data
        } else
            return -1; // error
    }
    *buf = '\0'; // null-terminate

    return n;
}
int recvdata(int fd, char *buf, int maxlen,struct sockaddr_in*client)
{
    socklen_t client_t = sizeof(struct sockaddr);
    return recvfrom(fd, buf, maxlen,0,(struct sockaddr *)client,&client_t);
}
