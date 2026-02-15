#pragma once

#include "LoggerManager.h"
#include <string>
#include <vector>

namespace pulse::net
{

template <typename T> class TCPMessageAssembler
{
  public:
    using MessageType = T;

    struct AssemblingResult
    {
        std::vector<MessageType> messages;
        bool error = false;
        std::string error_message;
    };

    ~TCPMessageAssembler() {};

    virtual AssemblingResult feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len) = 0;

    inline void log(SEVERITY severity, std::string_view message) const
    {
        LoggerManager::get_logger()->write(severity, message);
    }
};
} // namespace pulse::net
