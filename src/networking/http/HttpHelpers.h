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

struct HttpHeader
{
    // General headers
    static constexpr std::string_view CONNECTION = "Connection";
    static constexpr std::string_view DATE = "Date";
    static constexpr std::string_view CACHE_CONTROL = "Cache-Control";
    static constexpr std::string_view PRAGMA = "Pragma";
    static constexpr std::string_view VIA = "Via";

    // Request headers
    static constexpr std::string_view HOST = "Host";
    static constexpr std::string_view USER_AGENT = "User-Agent";
    static constexpr std::string_view ACCEPT = "Accept";
    static constexpr std::string_view ACCEPT_ENCODING = "Accept-Encoding";
    static constexpr std::string_view ACCEPT_LANGUAGE = "Accept-Language";
    static constexpr std::string_view AUTHORIZATION = "Authorization";
    static constexpr std::string_view CONTENT_LENGTH = "Content-Length";
    static constexpr std::string_view CONTENT_TYPE = "Content-Type";
    static constexpr std::string_view COOKIE = "Cookie";
    static constexpr std::string_view REFERER = "Referer";

    // Response headers
    static constexpr std::string_view SERVER = "Server";
    static constexpr std::string_view SET_COOKIE = "Set-Cookie";
    static constexpr std::string_view LOCATION = "Location";
    static constexpr std::string_view TRANSFER_ENCODING = "Transfer-Encoding";
    static constexpr std::string_view CONTENT_ENCODING = "Content-Encoding";
    static constexpr std::string_view WWW_AUTHENTICATE = "WWW-Authenticate";
};

constexpr std::string getStatusName(HttpStatus status)
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
