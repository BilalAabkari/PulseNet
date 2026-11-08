#pragma once
#include "../TCPMessageAssembler.h"
#include "TLSMessage.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <shared_mutex>
#include <source_location>
#include <unordered_map>

namespace pulse::net
{

class TLSAssembler : public TCPMessageAssembler<TLSMessage>
{
  public:
    TLSAssembler(const std::string &certificate_chain_file_path, const std::string &private_key_file_path);
    ~TLSAssembler() = default;

    virtual AssemblingResult feed(uint64_t id, uint8_t *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len) override;

    virtual void terminate(uint64_t id) override;

  private:
    struct ClientTLSState
    {

        ClientTLSState(int max_length)
            : length(0), decrypted_buffer(new uint8_t[max_length]), ssl(nullptr), pos(0), pending_accept(true) {};

        ClientTLSState(const ClientTLSState &) = delete;
        ClientTLSState operator=(const ClientTLSState &) = delete;

        int length;
        std::unique_ptr<uint8_t[]> decrypted_buffer;
        int pos;
        SSL *ssl;
        bool pending_accept;
    };

    void throwSSLError(const std::string &message_prefix) const;
    void SSLProtocolError(AssemblingResult &result, const std::string &error_message, uint64_t client_id,
                          int error_code, std::source_location loc) const;
    void flushContentsToClient(AssemblingResult &result, ClientTLSState &client_state, bool is_error);

    unsigned int m_buffer_max_len = 16 * 1024;

    std::unordered_map<uint64_t, ClientTLSState> m_client_states;
    std::shared_mutex m_mtx;
    SSL_CTX *m_ssl_ctx;

    static const std::string SSL_SESSION_ID;
};

} // namespace pulse::net
