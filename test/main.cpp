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

const char* request = "GET / HTTP/1.1\r\n"\
					  "HOST: 192.168.200.102\r\n"
					  "\r\n";
					  
int main(int argc,char** argv)
{
	int client_fd = -1;
	struct sockaddr_in serveraddr;
	HexDump dump;
	if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Error on socket creation\n");
		return -1;
	}
	
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("192.168.200.102");
	serveraddr.sin_port = htons(18443);
	
	connect(client_fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in));
	
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
		return 1;
	}
	SSLBase* ssl_client = factory.create_client();
	
	if (!ssl_client)
	{
		printf("ssl client init failed:%s\n",factory.get_error().c_str());
		close(client_fd);
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
		int ret = ssl_client->handshake((unsigned char*)buf,len,send);
		if (ret < 0)
			break;
	} while(!ssl_client->is_handshake_ok());
	
	if (!ssl_client->is_handshake_ok())
	{
		printf("##Error:%s\n",ssl_client->get_error().c_str());
		close(client_fd);
		return 1;
	}
	
	ssl_client->write((const unsigned char*)request,strlen(request),send);
	std::string output;
	output.reserve(1024 * 16);

	printf("%s\n",ssl_client->get_peer_cert().c_str());
	
	sleep(10);
	do {
		len = read(client_fd,buf,sizeof(buf));
		ssl_client->read((const unsigned char*)buf, len, std::bind([&output](const unsigned char* buffer,int length)
		{
			output.append((const char*)buffer,length);
		},std::placeholders::_1,std::placeholders::_2));
	} while(len > 0);
	printf("%s\n",output.c_str());
	delete ssl_client;
	
	return 0;
}