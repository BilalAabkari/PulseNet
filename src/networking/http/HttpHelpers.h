#pragma once

#include <string>

namespace pulse::net
{

enum class HttpStatus
{
    OK = 200,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404
};

enum class HttpVersion
{
    UNKNOWN,
    HTTP_1_0,
    HTTP_1_1,
    HTTP2
};

std::string getStatusName(HttpStatus status)
{
    switch (status)
    {
    case HttpStatus::OK:
        return "OK";
    case HttpStatus::BAD_REQUEST:
        return "Bad Request";
    case HttpStatus::UNAUTHORIZED:
        return "Unauthorized";
    case HttpStatus::FORBIDDEN:
        return "Forbidden";
    case HttpStatus::NOT_FOUND:
        return "Not Found";
    default:
        return "Unknown Status";
    }
}

} // namespace pulse::net
