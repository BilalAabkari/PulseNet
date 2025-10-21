#include "HttpResponse.h"

namespace pulse::net
{

HttpResponse::HttpResponse(HttpVersion version, HttpStatus status, std::string body)
    : m_version(version), m_status(status), m_body(body)
{
}

HttpResponse::~HttpResponse()
{
}

std::string HttpResponse::serialize() const
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

void HttpResponse::setHttpStatus(HttpStatus status)
{
    m_status = status;
}

void HttpResponse::setBody(std::string &&body)
{
    m_body = std::move(body);
}

void HttpResponse::addHeader(const std::string &name, const std::string &value)
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

} // namespace pulse::net
