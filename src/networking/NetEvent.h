#pragma once

#include <string>

namespace pulse::net
{

class NetEvent
{
  public:
    enum class Protocol
    {
        HTTP
    };

    virtual ~NetEvent() {};

    virtual NetEvent::Protocol protocol() = 0;
};

} // namespace pulse::net
