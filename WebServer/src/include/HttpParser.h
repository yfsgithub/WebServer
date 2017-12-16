#pragma once
#include "../include/HttpRequest.h"
#include "Header.h"
#include <winsock2.h>
#include <fstream>
#include <vector>

class HttpParser
{
public:
	static std::string getClientData(SOCKET clientInstance, int port, int clientID);
	static Request parseRequestData(char* toParse);
	static rMethod getRequestMethod(const std::string method);
};
