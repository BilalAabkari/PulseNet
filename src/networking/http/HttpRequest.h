#pragma once

#include "../NetEvent.h"

namespace pulse::net
{

class HttpRequest : NetEvent
{
  public:
    NetEvent::Protocol protocol();

  private:
};

} // namespace pulse::net
