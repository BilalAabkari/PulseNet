#pragma once

#include <string>
#include <vector>

namespace pulse::net
{

class TCPMessageAssembler
{
  public:
    ~TCPMessageAssembler() {};

    virtual std::vector<std ::string> feed(uint64_t id, char *buffer) = 0;
};
} // namespace pulse::net
