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

    AssemblingResult result;
    result.to_write.resize(m_buffer_max_len);      // This may need tweaking
    result.error_message.resize(m_buffer_max_len); // This may need tweaking

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

        SSL *ssl = SSL_new(m_ssl_ctx);

        if (ssl == NULL)
        {
            log(SEVERITY::INFO, "Cound not create SSL object for client " + std::to_string(id) +
                                    "\n Error code: " + std::to_string(ERR_peek_last_error()));

            flushContentsToClient(result, *client_state, true);
            return result;
        }

        BIO *r_client_bio = BIO_new(BIO_s_mem());
        BIO *w_client_bio = BIO_new(BIO_s_mem());
        SSL_set_bio(ssl, r_client_bio, w_client_bio);

        client_state->ssl = ssl;
    }

    size_t written_bytes;

    BIO *rbio = SSL_get_rbio(client_state->ssl);

    if (rbio == NULL)
    {
        log(SEVERITY::FATAL, "Read bio is not set for client " + std::to_string(id));
        flushContentsToClient(result, *client_state, true);
        return result;
    }

    if (BIO_write_ex(rbio, buffer + client_state->pos, buffer_len - client_state->pos, &written_bytes) != 1)
    {
        unsigned long err = ERR_peek_last_error();
        const char *message = ERR_reason_error_string(err);
        ERR_pop();

        log(SEVERITY::FATAL, std::string("Error writing client buffer to SSL BIO") +
                                 "\n\t Client id: " + std::to_string(id) + "\n\t SSL Error code " + message +
                                 "\n\t at " + std::source_location::current().file_name() + ":" +
                                 std::to_string(std::source_location::current().line()));

        flushContentsToClient(result, *client_state, true);
        return result;
    }

    client_state->pos += written_bytes;

    if (client_state->pos >= max_buffer_len)
    {

        client_state->pos = 0;
        buffer_len = 0;
    }

    if (client_state->pending_accept)
    {
        int accept_result = SSL_accept(client_state->ssl);
        if (accept_result == 1)
        {
            client_state->pending_accept = false;
        }
        else
        {
            int err = SSL_get_error(client_state->ssl, accept_result);

            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            {
                unsigned long error_code = ERR_peek_last_error();
                char error_buffer[1024];
                ERR_error_string_n(error_code, error_buffer, 1024);

                log(SEVERITY::FATAL, std::string("Error performing ssl handshake. ") + "\n\t Client id: " +
                                         std::to_string(id) + "\n\t SSL Error code " + std::to_string(err) +
                                         "\n\t at " + std::source_location::current().file_name() + ":" +
                                         std::to_string(std::source_location::current().line()));
                flushContentsToClient(result, *client_state, true);

                return result;
            }
        }
    }

    if (client_state->pending_accept == false)
    {

        size_t read_bytes = 0;

        int read_result = SSL_read_ex(client_state->ssl, client_state->decrypted_buffer.get() + client_state->length,
                                      m_buffer_max_len - client_state->length, &read_bytes);

        if (read_result <= 0)
        {
            int err = SSL_get_error(client_state->ssl, read_result);

            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            {

                log(SEVERITY::FATAL,
                    "Error reading data from the client ssl ecrypted buffer. Client id: " + std::to_string(id) +
                        "\n Error code: " + std::to_string(ERR_peek_last_error()));

                flushContentsToClient(result, *client_state, true);
                return result;
            }
        }
        else
        {
            client_state->length += read_bytes;
        }
    }

    // Before returning, we make sure that there is anything to send back to client
    flushContentsToClient(result, *client_state, false);

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

void TLSAssembler::SSLProtocolError(AssemblingResult &result, const std::string &error_message, uint64_t client_id,
                                    int error_code, std::source_location loc) const
{
    // ERR_print_errors_fp(stderr);

    log(SEVERITY::S_ERROR, error_message + "\n\t Client id: " + std::to_string(client_id) + "\n\t SSL Error code " +
                               std::to_string(error_code) + "\n\t at " + loc.file_name() + ":" +
                               std::to_string(loc.line()));
    ERR_pop();

    result.error = true;
}

void TLSAssembler::flushContentsToClient(AssemblingResult &result, ClientTLSState &client_state, bool is_error)
{
    BIO *wbio = SSL_get_wbio(client_state.ssl);
    size_t bytes_to_send = 0;

    BIO_read_ex(wbio, is_error ? result.error_message.data() : result.to_write.data(), result.to_write.size(),
                &bytes_to_send);

    if (bytes_to_send > 0)
    {
        if (is_error)
        {
            result.error = true;
            result.error_message.resize(bytes_to_send);
        }
        else
        {
            result.wants_to_write = true;
            result.to_write.resize(bytes_to_send);
        }
    }
}

void TLSAssembler::terminate(uint64_t id)
{
    std::shared_lock lock(m_mtx);
    auto it = m_client_states.find(id);
    lock.unlock();

    if (it != m_client_states.end())
    {
        SSL_free(it->second.ssl);
        std::unique_lock u_lock(m_mtx);
        m_client_states.erase(id);
        u_lock.unlock();
    }
}

} // namespace pulse::net
