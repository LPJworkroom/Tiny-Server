#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<fstream>
#include<string>
#include<queue>
#include<vector>
//#include<thread>
#include<sstream>	//alternate of itoa&&atoi
using namespace std;
#define SOCK int
const int useport=80,bufsize=1000;
const char myserverip[]="127.0.0.1";
const string pagebreak="\n------------------------------------------------\n";

void doweb(SOCK connfd);
void server_static(SOCK connfd,ifstream& fin,string& filepath);
string itoa(int i);
void send_error(SOCK connfd,string& answer);
void get_filetype(string& filepath,string& filetype);
void read_request(string& todo,string& answer,string& filepath);
void server_initial(int&);
void recv_write(SOCK connfd,string& dest);
void clienterror(string& dest,string code,string shortmsg,string longmsg);

int main()
{
	int s_listen;
	server_initial(s_listen);
	sockaddr clientaddr;
	unsigned clientaddrlen=sizeof(clientaddr);
	printf("listen..\n");
	int connfd;
	while ((connfd=accept(s_listen,(sockaddr*)&clientaddr,&clientaddrlen))>0)
	{
		printf("new acc..\n");
		doweb(connfd);
		printf("wait for next client...\n");
	}
	printf("warning:listen terminated\n");
return 0;
}

/*in another thread*/
void doweb(SOCK connfd)
{
	string strrecv,answer;
	/*read request*/
	recv_write(connfd,strrecv);
	cout<<"string recv:"<<endl<<strrecv<<pagebreak;
	/*parse string recved and check file*/
	string filepath;
	read_request(strrecv,answer,filepath);
	cout<<"request filepath:"<<endl<<filepath<<pagebreak;
	ifstream fin(filepath);
	/*file cannot be fetch*/
	if (answer.empty()&&!fin.is_open())	clienterror(answer,"404","Not found","Tiny couldn't find the file");
	/*success*/
	if (fin.is_open()&&!filepath.empty()){
		cout<<"prepare static content.."<<pagebreak;
		server_static(connfd,fin,filepath);
	}
	else{
		cout<<"wrong request,reply about error"<<pagebreak;
		send(connfd,answer.c_str(),answer.size(),0);
	}
	
	//close(connfd);
	/*shutdown() is needed:close() only close socket
	 *in this thread,socket main thread is not closed,
	 *while shutdown close socket in all thread.
	 */
	shutdown(connfd,SHUT_RDWR);	//shut both write and read
}

void clienterror(string& dest,string code,string shortmsg,string longmsg)
{
	dest="<html><title>Tiny Error</title>";
	dest+="<body bgcolor=""ffffff"">\r\n";
	dest+=code+':'+shortmsg+"\r\n";
	dest+="<p>"+longmsg+"\r\n";
	dest+="<hr><em>The Tiny Web Server</em>\r\n";
}

void server_static(SOCK connfd,ifstream& fin,string& filepath)
{
	string output,filetype,text;
	get_filetype(filepath,filetype);
	char ch;
	while (!fin.eof())
	{
		fin.get(ch);
		text.push_back(ch);
	}
	
	output+="HTTP/1.0 200 OK\r\n";
	output+="Server:Tiny web server\r\n";
	output+="Connection:close\r\n";
	output+="Content-length:"+itoa(text.size())+"\r\n";
	output+="Content-type:"+filetype+"\r\n\r\n";
	output+=text;
	
	send(connfd,output.c_str(),output.size(),0);
}

string itoa(int i)
{
	stringstream ss;
	ss<<i;
	string ret;
	ss>>ret;
	return ret;
}

/*filetype string in http header*/
void get_filetype(string& filepath,string& filetype)
{
	if (filepath.find(".html")!=string::npos)	filetype="text/html";
	else	if (filepath.find(".gif")!=string::npos)	filetype="image/gif";
	else	if (filepath.find(".png")!=string::npos)	filetype="image/png";
	else	if (filepath.find(".jpg")!=string::npos)	filetype="image/jpeg";
	else	if (filepath.find(".js")!=string::npos)	filetype="text/javascript";
	else	if (filepath.find(".css")!=string::npos)	filetype="text/css";
	else	filetype="text/plain";
}

void read_request(string& todo,string& answer,string& filepath)
{
	int lbound=todo.find("GET");
	if (lbound==string::npos){
		clienterror(answer,"501","Not implemented","Tiny doesn't implement this method");
		return;
	}
	//lbound+=strlen("GET");
	lbound=todo.find('/',lbound);
	int rbound=todo.find(' ',lbound);
	filepath='.';
	filepath+=todo.substr(lbound,rbound-lbound);
	/*default: index*/
	if (filepath.back()=='/')	filepath+="index.html";
	return;
}

void recv_write(SOCK connfd,string& dest)
{
	int len,recv_size=0;
	char tbuf[bufsize];
	memset(tbuf,0,sizeof(tbuf));
	while ((len=recv(connfd,tbuf,bufsize-1,0))>0)
	{
		printf("%d\n",len);
		string tmp(tbuf,len);
		dest+=tmp;
		recv_size+=len;
		if (dest.find("\r\n\r\n",max(len-2*bufsize,0))!=string::npos){
			break;
		}
	}
	printf("over.recv size:%d\n",len);
}

void server_initial(int& s_listen)
{
	if (((s_listen=socket(AF_INET,SOCK_STREAM,0))<0))
		printf("wrong socket\n");
	
	int bReuseaddr=1;
	if (setsockopt(s_listen,SOL_SOCKET ,SO_REUSEADDR,(const char*)&bReuseaddr,sizeof(bReuseaddr))<0)
		printf("setsockpot wrong\n");
		
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(useport);
	if (bind(s_listen,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		printf("wrong bind\n");
		perror("bind");
	}
	if (listen(s_listen,1000)<0)
	{
		printf("wrong listen\n");
		perror("listen");
	}
	printf("server initial ok\n");
}
