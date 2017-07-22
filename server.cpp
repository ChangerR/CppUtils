#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "ssl_util.h"
#include "Hexdump.h"

int main(int argc,char** argv)
{
	int listen_fd = -1;
	struct sockaddr_in serveraddr;
	struct sockaddr clientaddr;
	socklen_t clientaddr_len = sizeof(clientaddr);
	HexDump dump;
	
	if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error on socket creation\n");
		return 1;
	}
	
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(6000);
	
	if (bind(listen_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		printf("Bind socket occur error,please check 6000 port\n");
		close(listen_fd);
		return 1;
	}
	
	if (listen(listen_fd,1) < 0)
	{
		printf("Bind socket occur error,please check 6000 port\n");
		close(listen_fd);
		return 1;
	}
	
	int client_fd = accept(listen_fd,&clientaddr,&clientaddr_len);
	if (client_fd < 0)
	{
		printf("accept client error\n");
		close(listen_fd);
		return 1;
	}
	
	if (fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
	{
		printf("call fcntl nonblocking failed");
		close(client_fd);
		return 1;
	}
	
	SSLFactory factory;
	if (!factory.load("CHANGER-PC.cert","CHANGER-PC.key"))
	{
		printf("ssl client init failed:%s\n",factory.get_error().c_str());
		close(client_fd);
		close(listen_fd);
		return 1;
	}
	SSLBase* ssl_server = factory.create_client();
	
	if (!ssl_server)
	{
		printf("ssl client init failed:%s\n",factory.get_error().c_str());
		close(client_fd);
		close(listen_fd);
		return 1;
	}
	
	auto send = std::bind([&client_fd,&dump](const unsigned char* buffer,int length)
	{
		dump.hexdump(buffer,length,std::bind([](const char* pbuf) { printf("%s\n",pbuf); },std::placeholders::_1));
		write(client_fd,buffer,length);
	},std::placeholders::_1,std::placeholders::_2);
	
	
	char buf[1024] = {0} ;
	int len = 0;
	do {
		len = read(client_fd,buf,sizeof(buf));
		if (len > 0)
		{
			dump.hexdump(buf,len,std::bind([](const char* pbuf) { printf("%s\n",pbuf); },std::placeholders::_1));
		}
		int ret = ssl_server->handshake((unsigned char*)buf,len,send);
		if (ret < 0)
			break;
	} while(!ssl_server->is_handshake_ok());
	
	if (!ssl_server->is_handshake_ok())
	{
		printf("##Error:%s\n",ssl_server->get_error().c_str());
		close(client_fd);
		close(listen_fd);
		return 1;
	}

	printf("%s\n",ssl_server->get_peer_cert().c_str());
	
	delete ssl_server;
	close(client_fd);
	close(listen_fd);
	return 0;
}