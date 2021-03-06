#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>

#include "../include/server.h"
#include "../include/logger.h"

std::string Logger::path = BASE_DIR + "log.txt";

http::Server::Server(int argc, char* argv[])
{
	std::string PORT, IP;
	try
	{
		if (argc > 1)
		{
			if (std::string(argv[1]) == "runserver")
			{
				if (argc > 3)
				{
					this->ip = argv[2];
					this->port = (uint16_t)std::stoi(argv[3]);
				}
				else
				{
					this->ip = SERVER_IP;
					this->port = (uint16_t)std::stoi(SERVER_PORT);
				}
			}
			else
			{
				std::cerr << "Invalid arguments passed...";
			}
		}
		else
		{
			std::cerr << "Invalid arguments passed...";
		}
	}
	catch (const std::invalid_argument&)
	{
		Logger::log()->error("'http::Server::Server()': 'invalid port number'", __LINE__ - 24, this->lockPrint);
		this->ip = SERVER_IP;
		this->port = (uint16_t)std::stoi(SERVER_PORT);
	}
	catch (...)
	{
		Logger::log()->error("'http::Server::Server()': 'unknown error occurred'", __LINE__, this->lockPrint);
		this->ip = SERVER_IP;
		this->port = (uint16_t)std::stoi(SERVER_PORT);
	}
	this->setApp();
}

void http::Server::setApp(Application* app)
{
	if (app)
	{
		this->app = app;
	}
}

void http::Server::start()
{
	WSA_STARTUP;
	std::thread newListenThread(&Server::startListener, this);
	if (newListenThread.joinable())
	{
		newListenThread.join();
	}
	WSA_CLEANUP;
}

void http::Server::startListener()
{
	SOCK listenSock;
	sockaddr_in addr = {};
	socklen_t sa_size = sizeof(sockaddr_in);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(this->port);
	inet_pton(AF_INET, this->ip.c_str(), &(addr.sin_addr));
	if ((listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCK)
	{
		Logger::log()->error("'http::Server::startThread()': 'socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)' failed.", __LINE__ - 2, this->lockPrint);
		WSA_CLEANUP;
		return;
	}
	if (bind(listenSock, (sockaddr*)&addr, sizeof(sockaddr_in)) == SOCK_ERROR)
	{
		Logger::log()->error("'http::Server::startThread()': 'bind(listenSock, (sockaddr*)&addr, sizeof(sockaddr_in))' failed.", __LINE__ - 2, this->lockPrint);
		if (CLOSE_SOCK(listenSock) == SOCK_ERROR)
		{
			Logger::log()->error("'http::Server::startThread()': 'closesocket(listenSock)' failed.", __LINE__ - 2, this->lockPrint);
		}
		WSA_CLEANUP;
		return;
	}
	if (listen(listenSock, SOMAXCONN) == SOCK_ERROR)
	{
		Logger::log()->error("'http::Server::startThread()': 'listen(listenSock, SOMAXCONN)' failed.", __LINE__ - 2, this->lockPrint);
		return;
	}
	bool listening = true;
	PRINT_SERVER_DATA(std::cout, Parser::getIP(listenSock), this->port);
	SOCK client;
	while (listening)
	{
		try
		{
			if ((client = accept(listenSock, (sockaddr*)&addr, &sa_size)) != INVALID_SOCK)
			{
				std::thread newClient(&Server::serveClient, this, client);
				newClient.detach();
			}
			else
			{
				Logger::log()->error("'http::Server::startThread()': invalid client socket.", __LINE__ - 7, this->lockPrint);
			}
		}
		catch (const std::exception& exc)
		{
			Logger::log()->error("'http::Server::startThread()': " + std::to_string(*exc.what()) + ".", __LINE__, this->lockPrint);
			listening = false;
		}
		catch (const char* exc)
		{
			Logger::log()->error("'http::Server::startThread()': " + std::to_string(*exc) + ".", __LINE__, this->lockPrint);
			listening = false;
		}
		catch (...)
		{
			Logger::log()->error("'http::Server::startThread()': unknown error occurred.", __LINE__, this->lockPrint);
			listening = false;
		}
	}
}

void http::Server::serveClient(const SOCK client)
{
	clock_t start, finish;
	start = clock();
	this->processRequest(client);
	finish = clock();
	float servingTime = (float)(finish - start) / CLOCKS_PER_SEC;
	std::ostringstream ss;
	ss << '[';
	DATE_TIME_NOW(ss, "%d/%b/%Y %r");
	ss << ']';
	ss << "\nRequest took: " + std::to_string(servingTime) + " seconds.\n\n";
	Logger::log()->file(ss.str(), this->lockPrint);
}

void http::Server::processRequest(const SOCK& client)
{
	char buffer[MAX_BUFF_SIZE];
	MSG_SIZE recvMsgSize;
	int bufError;
	std::string data;
	unsigned long size = 0;
	do
	{
		recvMsgSize = recv(client, buffer, MAX_BUFF_SIZE, 0);
		if (recvMsgSize > 0)
		{
			data += buffer;
			size += recvMsgSize;
			data = data.substr(0, size);
		}
		else if (recvMsgSize < 0)
		{
			Logger::log()->error("'http::Server::processRequest()': 'recv(client, buffer, 1024, 0)' failed.", __LINE__ - 9, this->lockPrint);
		}
	} while (recvMsgSize >= MAX_BUFF_SIZE);
	try
	{
		if (!data.empty())
		{
			Request request(Request::Parser::parseRequestData((char*)data.c_str(), this->lockPrint, Parser::getIP(client)));
			this->sendResponse(request, client);
		}
		else
		{
			this->sendFile(Response::BadRequest(), client);
		}
	}
	catch (...)
	{
		this->sendFile(Response::BadRequest(), client);
	}
	this->closeSocket(client, SOCK_SEND, "processRequest()", "shutdown(client, SOCK_SEND)", __LINE__);
	this->closeSocket(client, SOCK_RECEIVE, "processRequest()", "shutdown(client, SOCK_RECEIVE)", __LINE__);
	bufError = CLOSE_SOCK(client);
	if (bufError == SOCK_ERROR)
	{
		Logger::log()->error("'http::Server::processRequest()': 'CLOSE_SOCK(client)' failed.", __LINE__ - 3, this->lockPrint);
		WSA_CLEANUP;
	}
}

void http::Server::sendResponse(Request& request, const SOCK& client)
{
	std::string response;
	std::string url = request.DATA.get("url");
	if (this->app)
	{
		if (Parser::requestStatic(url))
		{
			if (this->app->hasStatic(url))
			{
				response = Response::responseStatic(this->app->createStaticDir(url));
			}
			else
			{
				response = Response::NotFound();
			}
		}
		else
		{
			if (this->app->checkUrl(url))
			{
				response = this->app->getFunction(url)(request);
			}
			else
			{
				response = Response::NotFound();
			}
		}
	}
	else
	{
		response = Response::InternalServerError();
	}
	this->sendFile(response, client);
}

void http::Server::sendFile(const std::string& httpResponse, const SOCK& client)
{
	if (send(client, httpResponse.c_str(), (int)httpResponse.size(), 0) == SOCK_ERROR)
	{
		Logger::log()->error("'http::Server::sendFile()': 'send(client, httpResponse.c_str(), (int)httpResponse.size(), 0)' failed.", __LINE__ - 2, this->lockPrint);
		if (CLOSE_SOCK(client) == SOCK_ERROR)
		{
			Logger::log()->error("'http::Server::sendFile()': 'CLOSE_SOCK(client)' failed.", __LINE__ - 2, this->lockPrint);
		}
		WSA_CLEANUP;
	}
}

void http::Server::closeSocket(const SOCK& sock, const int how, const std::string& method, const std::string& func, const int line)
{
	if (shutdown(sock, how) == SOCK_ERROR)
	{
		Logger::log()->error("'http::Server::" + method + "': '" + func + "' failed.", line, this->lockPrint);
		if (CLOSE_SOCK(sock) == SOCK_ERROR)
		{
			Logger::log()->error("'http::Server::closeSocket()': 'CLOSE_SOCK(sock)' failed.", __LINE__ - 2, this->lockPrint);
		}
		WSA_CLEANUP;
	}
}
