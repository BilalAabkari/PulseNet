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

    virtual ~HttpMessage();

    virtual std::string serialize() const;

    void setHttpStatus(HttpStatus status);

    void setBody(std::string &&body);

    void addHeader(const std::string &name, const std::string &value);

  private:
    std::unordered_map<std::string, std::string> m_headers;
    HttpType m_type = HttpType::UNINITALIZED;
    HttpStatus m_status = HttpStatus::NONE;
    HttpVersion m_version = HttpVersion::UNKNOWN;
    std::string m_body;
};

} // namespace pulse::net
