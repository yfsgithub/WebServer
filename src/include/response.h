#pragma once

#ifndef WEB_SERVER_RESPONSE_H
#define WEB_SERVER_RESPONSE_H

#include "headers.h"
#include <string>

__HTTP_BEGIN

class Response
{
public:
	static std::string HttpResponse(const std::string& path, const std::string& status = "OK", size_t statusCode = 200);
	static std::string NotFound();
	static std::string Forbidden();
	static std::string MethodNotAllowed();
	static std::string InternalServerError();
	static std::string BadRequest();
	static std::string responseStatic(const std::string& fileName);
private:
	class Parser
	{
	public:
		static std::string readFile(const std::string& filePath, bool openBinary = false);
		static std::string errorPage(size_t code, const std::string& msg);
		static std::string makeResponse(const std::string& content, const std::string& statusStr, size_t statusCode, const std::string& requestContent);
		static std::string setContentType(const std::string& requestContent);
	};
};

__HTTP_END

#endif
