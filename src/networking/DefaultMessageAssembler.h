#pragma once

#include "TCPMessageAssembler.h"

namespace pulse::net
{

class DefaultMessageAssembler : public TCPMessageAssembler
{
  public:
    ~DefaultMessageAssembler() = default;

    virtual std::vector<std ::string> feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                           int last_tcp_packet_len)
    {

        std::vector<std::string> messages;
        if (buffer_len > 0 && buffer != nullptr)
        {
            messages.emplace_back(buffer, buffer_len);
            buffer_len = 0;
        }

        return messages;
    }
};

} // namespace pulse::net
