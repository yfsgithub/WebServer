#include "../include/HttpParser.h"
#include "../include/RegularExpressions.h"
#include <ws2tcpip.h>
#include <regex>
#include <iostream>

std::string HttpParser::getClientData(SOCKET clientInstance, int port, int clientID)
{
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int res = getsockname(clientInstance, (struct sockaddr *)&addr, &addr_size);
	sockaddr_in* pV4Addr = (struct sockaddr_in*)&addr;
	int ipAddr = pV4Addr->sin_addr.s_addr;
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, clientIp, INET_ADDRSTRLEN);
	std::string result("ID: " + std::to_string(clientID) + "\nThe Client port is: " + 
		std::to_string(port) + "\nThe Client IP is: " + clientIp + '\n');
	return result;
}

Request HttpParser::parseRequestData(char* toParse)
{
	std::string firstLine("");
	while (*toParse != '\r')
	{
		firstLine += *toParse;
		toParse++;
	}
	toParse += 2;
	std::cout << "\nClient request: " << firstLine << '\n';
	std::string url(""), method("");
	std::smatch data;
	if (std::regex_match(firstLine, data, std::regex(REGEX::FIRST_LINE_REQUEST)))
	{
		method = data[1].str();
		url = data[2].str();
	}
	std::string body(toParse);
	body = std::regex_replace(body, std::regex("\\r+"), "");
	return Request(body, method, url);
}

rMethod HttpParser::getRequestMethod(const std::string method)
{
	rMethod result = rMethod::None;
	std::string methodToLower(method);
	std::transform(methodToLower.begin(), methodToLower.end(), methodToLower.begin(), ::tolower);
	if (methodToLower == "get")
	{
		result = rMethod::Get;
	}
	else if (methodToLower == "post")
	{
		result = rMethod::Post;
	}
	else if (methodToLower == "put")
	{
		result = rMethod::Put;
	}
	else if (methodToLower == "delete")
	{
		result = rMethod::Delete;
	}
	return result;
}
