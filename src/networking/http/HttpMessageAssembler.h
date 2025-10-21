#pragma once
#include "../TCPMessageAssembler.h"
#include "HttpHelpers.h"
#include <mutex>
#include <unordered_map>

namespace pulse::net
{

class HttpMessageAssembler : public TCPMessageAssembler
{
  public:
    HttpMessageAssembler();

    virtual std::vector<std::string> feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                          int last_tcp_packet_len);

  private:
    struct HttpStreamState
    {
        int body_lenght = -1;
        bool headers_read = false;
        HttpVersion http_version = HttpVersion::UNKNOWN;
        std::unordered_map<std::string, std::string> headers;
    };

    std::unordered_map<uint64_t, HttpStreamState> m_client_states;
    std::mutex m_mtx;
};

} // namespace pulse::net
