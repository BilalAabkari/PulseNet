#pragma once

#include "LoggerManager.h"
#include <cstdint>
#include <memory>
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
        std::vector<std::shared_ptr<MessageType>> messages;
        bool error = false;
        std::string error_message;

        std::string to_write;
        bool wants_to_write = false;
    };

    ~TCPMessageAssembler() {};

    virtual AssemblingResult feed(uint64_t id, uint8_t *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len) = 0;

    virtual void terminate(uint64_t id) = 0;

    inline void log(SEVERITY severity, std::string_view message) const
    {
        LoggerManager::get_logger()->write(severity, message);
    }
};
} // namespace pulse::net
