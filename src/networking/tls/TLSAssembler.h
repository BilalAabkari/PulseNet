#pragma once
#include "../TCPMessageAssembler.h"
#include "TLSMessage.h"
#include <shared_mutex>
#include <unordered_map>

namespace pulse::net
{

class TLSAssembler : TCPMessageAssembler<TLSMessage>
{
  public:
    TLSAssembler();
    ~TLSAssembler() = default;

    virtual AssemblingResult feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len) = 0;

  private:
    struct ClientTLSState
    {
        int lenght;
        char *decrypted_buffer;
    };

    std::unordered_map<uint64_t, ClientTLSState> m_client_states;
    std::shared_mutex m_mtx;
};

} // namespace pulse::net
