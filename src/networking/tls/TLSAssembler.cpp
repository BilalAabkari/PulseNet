#include "TLSAssembler.h"

namespace pulse::net
{

const std::string TLSAssembler::SSL_SESSION_ID = "pulsenet";

TLSAssembler::TLSAssembler(const std::string &certificate_chain_file_path, const std::string &private_key_file_path)
{
    m_ssl_ctx = SSL_CTX_new(TLS_server_method()); // TODO: destructor

    if (m_ssl_ctx == nullptr)
        throwSSLError("Error initializing SSL context.");

    if (!SSL_CTX_set_min_proto_version(m_ssl_ctx, TLS1_2_VERSION))
        throwSSLError("Error setting TLS min version");

    SSL_CTX_set_options(m_ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION | SSL_OP_SERVER_PREFERENCE);

    if (SSL_CTX_use_certificate_chain_file(m_ssl_ctx, certificate_chain_file_path.c_str()) <= 0)
        throwSSLError("Error reading and using certificate chain file from \"" + certificate_chain_file_path + "\"");

    if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, private_key_file_path.c_str(), SSL_FILETYPE_PEM) <= 0)
        throwSSLError("Error reading and using private key file from " + private_key_file_path + "\"");

    SSL_CTX_set_session_id_context(m_ssl_ctx, reinterpret_cast<const unsigned char *>(SSL_SESSION_ID.c_str()),
                                   SSL_SESSION_ID.length());

    SSL_CTX_sess_set_cache_size(m_ssl_ctx, SSL_SESSION_CACHE_MAX_SIZE_DEFAULT);

    SSL_CTX_set_timeout(m_ssl_ctx, 3600);
    SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_NONE, NULL);
}

TLSAssembler::AssemblingResult TLSAssembler::feed(uint64_t id, uint8_t *buffer, int &buffer_len, int max_buffer_len,
                                                  int last_tcp_packet_len)
{
    ClientTLSState *client_state;

    std::shared_lock lock(m_mtx);

    auto it = m_client_states.find(id);

    if (it != m_client_states.end())
    {
        client_state = &it->second;
        lock.unlock();
    }
    else
    {
        lock.unlock();
        std::unique_lock u_lock(m_mtx);
        client_state = &m_client_states.emplace(id, m_buffer_max_len).first->second;
        u_lock.unlock();
    }

    AssemblingResult result;

    return result;
}

void TLSAssembler::throwSSLError(const std::string &message_prefix) const
{
    unsigned long error_code = ERR_peek_last_error();

    char buffer[1024];
    ERR_error_string_n(error_code, buffer, 1024);

    std::string message = message_prefix + " \n Error code: " + std::to_string(error_code) +
                          "\n Error message: " + buffer + "\n " + ERR_reason_error_string(error_code);

    ERR_pop();
    LoggerManager::get_logger()->write(SEVERITY::FATAL, message);
    throw new std::runtime_error(message);
}

void TLSAssembler::terminate(uint64_t id)
{
}

} // namespace pulse::net
