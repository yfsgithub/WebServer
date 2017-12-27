#include "../include/server.h"
#include <regex>
#include <iostream>

std::string http::Server::Parser::getClientData(SOCK client, uint16_t port, int clientID)
{
	std::string result = "ID: " + std::to_string(clientID) + "\nThe Client port is: ";
	result += std::to_string(port);
	result += "\nThe Client IP is: ";
	result += Parser::getIP(client);
	result += '\n';
	return result;
}
View* http::Server::Parser::availableView(std::vector<View*> views, const std::string url)
{
	for (const auto& view : views)
	{
		if (view->urlIsAvailable(url))
		{
			return view;
		}
	}
	return nullptr;
}
std::string http::Server::Parser::getIP(SOCK socket)
{
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getsockname(socket, (struct sockaddr *)&addr, &addr_size);
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, clientIp, INET_ADDRSTRLEN);
	return clientIp;
}
bool http::Server::Parser::requestStatic(const std::string url)
{
	return url.find('.') != std::string::npos;
}
