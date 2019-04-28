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
#include<thread>
#include<algorithm>
#include<sstream>	//alternate of itoa&&atoi
using namespace std;
#define SOCK int
const int useport=80,bufsize=10000;
const string pagebreak="\n------------------------------------------------\n";
string mobiles[7]={"android","webos","iphone","ipod","blackberry","iemobile","opera mini"};
struct waitnode{
	sockaddr addr;
	SOCK connfd;
};
struct timeval timeout={0,1000*100};

queue<SOCK> sockq;	//store recved request socket


bool ismobile(string& todo);
char mytolower(char a);
void take_request();
void take_file(SOCK connfd,string& strrecv);
void server_static(SOCK connfd,ifstream& fin,string& filepath);
string itoa(int i);
void send_error(SOCK connfd,string& answer);
void get_filetype(string& filepath,string& filetype);
void read_request(string& todo,string& answer,string& filepath);
void server_initial(int&);
int recv_write(SOCK connfd,string& dest);
void clienterror(string& dest,string code,string shortmsg,string longmsg);
void set_recv_tout(SOCK connfd);

int main()
{
	int s_listen;
	server_initial(s_listen);
	sockaddr clientaddr;
	unsigned clientaddrlen=sizeof(clientaddr);
	printf("listen..\n");
	
	thread doweb(take_request);
	doweb.detach();
	
	int connfd;
	while ((connfd=accept(s_listen,(sockaddr*)&clientaddr,&clientaddrlen))>0)
	{
		sockq.push(connfd);
	}
	printf("warning:listen terminated\n");
return 0;
}

/*in another thread*/
void take_request()
{
	while (1)
	{
		if (sockq.empty()){
			usleep(1000*100);
			continue;
		}
		int connfd=sockq.front();sockq.pop();
		string strrecv;
		recv_write(connfd,strrecv);
		cout<<"string recv:"<<endl<<strrecv<<pagebreak;	
		take_file(connfd,strrecv);
		close(connfd);
	}
}

void take_file(SOCK connfd,string& strrecv)
{
	/*parse string recved and check file*/
	string filepath,answer;
	read_request(strrecv,answer,filepath);
	cout<<"request filepath:"<<endl<<filepath<<pagebreak;
	ifstream fin;
	fin.open(filepath); 
	/*file cannot be fetch*/
	if (answer.empty()&&!fin.is_open()){
		sleep(1);
		fin.open(filepath);
		if (!fin.is_open())
			clienterror(answer,"404","Oops！","TinyServer正忙！\n请刷新本页面，或检查URL是否正确。\n(x_x)\n");
	}
	/*success*/
	if (fin.is_open()&&!filepath.empty()){
		cout<<"prepare static content.."<<pagebreak;
		server_static(connfd,fin,filepath);
	}
	else{
		cout<<"wrong request,reply about error"<<pagebreak;
		send(connfd,answer.c_str(),answer.size(),0);
	}
	if (fin.is_open())	fin.close(); 
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
	/*short connect or keep-alive?*/
	output+="Connection:close\r\n";
	output+="Content-length:"+itoa(text.size())+"\r\n";
	output+="Content-type:"+filetype+"\r\n\r\n";
	output+=text;
	
	if (send(connfd,output.c_str(),output.size(),0)<0){
		cout<<"send static content failed."<<endl;
		perror("send");
		cout<<pagebreak;
	}
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
	if (todo.empty())	return;
	int lbound=todo.find("GET");
	if (lbound==string::npos){
		clienterror(answer,"501","Not implemented","Tiny doesn't implement this method");
		return;
	}
	lbound=todo.find('/',lbound);
	int rbound=todo.find(' ',lbound);
	filepath+=todo.substr(lbound,rbound-lbound);
	/*default: index
	 *special check for mobile agent
	 */ 
	if (filepath.back()=='/')	filepath+="index.html";
	if (ismobile(todo))	filepath="./mobile"+filepath;
	else				filepath="."+filepath;
	return;
}

bool ismobile(string& todo)
{	
	string tmp=todo;
	int l=tmp.find("User-Agent:"),r=tmp.find("\r\n",l);
	tmp=tmp.substr(l,r-l);
	transform(tmp.begin(),tmp.end(),tmp.begin(),mytolower);
	cout<<"transformed string:"<<tmp<<pagebreak;
	for (int i=0;i<7;i++)
	{
		if (tmp.find(mobiles[i])!=string::npos){
		//	cout<<"clinet is using "<<mobiles[i]<<pagebreak;
			return true;
		}
	}
	return false;
}

char mytolower(char a)
{
	if (a>='A'&&a<='Z')	return (a+('a'-'A'));
	return a;
}

void set_recv_tout(SOCK connfd)
{
	int ret=setsockopt(connfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
	if (ret<0){
		cout<<"wrong setsockopt recv";
		perror("setsockopt");
		cout<<pagebreak;
	}
}

int recv_write(SOCK connfd,string& dest)
{
	int len,recv_size=0;
	char tbuf[bufsize];
	memset(tbuf,0,sizeof(tbuf));
	dest.clear();
	
	set_recv_tout(connfd);
	
	while ((len=recv(connfd,tbuf,bufsize-1,0))>=0)
	{
		printf("%d\n",len);
		if (len==0){
			cout<<"I think client closed connection."<<pagebreak;
	//		close(connfd);
			break;
		}
		string tmp(tbuf,len);
		dest+=tmp;
		recv_size+=len;
		if (dest.find("\r\n\r\n",max(recv_size-2*bufsize,0))!=string::npos){
			break;
		}
		
	}
	if (len<0){
		cout<<"wrong recv.";
		perror("recv");
		cout<<pagebreak;
		return len;
	}
	printf("over.recv size:%d\n",recv_size);
	return recv_size;
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
