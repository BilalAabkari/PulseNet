#pragma once
#include "HttpHelpers.h"
#include "JSONSerializable.h"
#include <sstream>
#include <unordered_map>

namespace pulse::net
{

class HttpMessage : public JSONSerializable
{

  public:
    HttpMessage() = default;
    HttpMessage(HttpVersion version, HttpStatus status, std::string body);
    HttpMessage(HttpVersion version, HttpMethod method, std::string &&uri,
                std::unordered_map<std::string, std::string> &&headers, std::string &&body);

    virtual ~HttpMessage();

    virtual std::string serialize() const;

    void setHttpStatus(HttpStatus status);

    void setBody(std::string &&body);

    void addHeader(const std::string &name, const std::string &value);

  private:
    std::unordered_map<std::string, std::string> m_headers;
    HttpType m_type = HttpType::UNKNOWN;
    HttpStatus m_status = HttpStatus::NONE;
    HttpMethod m_method = HttpMethod::UNKNOWN;
    HttpVersion m_version = HttpVersion::UNKNOWN;
    std::string m_body;
    std::string m_uri;

  private:
    std::string serializeRequest() const;
    std::string serializeResponse() const;
    constexpr std::string parseMethod(HttpMethod method) const noexcept;
};

} // namespace pulse::net
