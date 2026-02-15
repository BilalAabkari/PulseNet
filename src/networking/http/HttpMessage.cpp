#include "HttpMessage.h"

namespace pulse::net
{

HttpMessage::HttpMessage(HttpVersion version, HttpStatus status, std::string body)
    : m_version(version), m_status(status), m_body(body)
{
    addHeader("Content-Length", std::to_string(body.size()));
    m_type = HttpType::RESPONSE;
}

HttpMessage::HttpMessage(HttpVersion version, HttpMethod method, std::string uri,
                         std::unordered_map<std::string, std::string> headers, std::string body)
    : m_version(version), m_method(method), m_body(std::move(body)), m_uri(std::move(uri)),
      m_headers(std::move(headers))
{
    m_type = HttpType::REQUEST;
}

HttpMessage::HttpMessage(HttpVersion version, HttpMethod method, std::string uri,
                         std::unordered_map<std::string, std::string> headers)
    : m_version(version), m_method(method), m_uri(std::move(uri)), m_headers(std::move(headers))
{
    m_type = HttpType::REQUEST;
}

HttpMessage::~HttpMessage()
{
}

std::string HttpMessage::serialize() const
{
    if (m_type == HttpType::REQUEST)
    {
        return serializeRequest();
    }
    else if (m_type == HttpType::RESPONSE)
    {
        return serializeResponse();
    }
    else
    {
        return "UNKOWN METHOD";
    }
}

void HttpMessage::setHttpStatus(HttpStatus status)
{
    m_status = status;
}

void HttpMessage::setBody(std::string &&body)
{
    m_body = std::move(body);
}

void HttpMessage::addHeader(const std::string &name, const std::string &value)
{
    auto it = m_headers.find(name);
    if (it != m_headers.end())
    {
        m_headers[name] = it->second + "," + value;
    }
    else
    {
        m_headers[name] = value;
    }
}

std::string HttpMessage::serializeRequest() const
{
    std::ostringstream oss;

    oss << parseMethod(m_method) << " " << m_uri;

    switch (m_version)
    {
    case HttpVersion::HTTP_1_0:
        oss << " HTTP/1.0";
        break;
    case HttpVersion::HTTP_1_1:
        oss << " HTTP/1.1";
        break;
    default:
        oss << " HTTP/1.1";
        break;
    }

    oss << "\r\n";

    for (auto header : m_headers)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "\r\n";
    oss << m_body;

    return oss.str();
}

std::string HttpMessage::serializeResponse() const
{
    std::ostringstream oss;

    switch (m_version)
    {
    case HttpVersion::HTTP_1_0:
        oss << "HTTP/1.0 ";
        break;
    case HttpVersion::HTTP_1_1:
        oss << "HTTP/1.1 ";
        break;
    default:
        oss << "HTTP/1.1 ";
        break;
    }

    oss << static_cast<int>(m_status) << " " << getStatusName(m_status) << "\r\n";

    for (auto header : m_headers)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    oss << "\r\n";
    oss << m_body;

    return oss.str();
}

constexpr std::string HttpMessage::parseMethod(HttpMethod method) const noexcept
{
    switch (method)
    {
    case HttpMethod::GET:
        return "GET";
    case HttpMethod::POST:
        return "POST";
    case HttpMethod::PUT:
        return "PUT";
    case HttpMethod::HTTP_DELETE:
        return "DELETE";
    case HttpMethod::PATCH:
        return "PATCH";
    case HttpMethod::TRACE:
        return "TRACE";
    case HttpMethod::HEAD:
        return "HEAD";
    case HttpMethod::OPTIONS:
        return "OPTIONS";
    case HttpMethod::CONNECT:
        return "CONNECT";
    case HttpMethod::INVALID:
        return "INVALID";
    case HttpMethod::UNKNOWN:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace pulse::net
