#pragma once
#include "HttpHelpers.h"
#include "JSONSerializable.h"
#include <sstream>
#include <unordered_map>

namespace pulse::net
{

class HttpResponse : public JSONSerializable
{

  public:
    HttpResponse(HttpVersion version, HttpStatus status, std::string body);

    virtual ~HttpResponse();

    virtual std::string serialize() const;

    void setHttpStatus(HttpStatus status);

    void setBody(std::string &&body);

    void addHeader(const std::string &name, const std::string &value);

  private:
    std::unordered_map<std::string, std::string> m_headers;
    HttpStatus m_status;
    HttpVersion m_version;
    std::string m_body;
};

} // namespace pulse::net
