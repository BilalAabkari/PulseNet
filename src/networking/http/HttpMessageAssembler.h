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
    HttpMessageAssembler(bool assemble_chunked_requests = true);

    virtual std::vector<std::string> feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                          int last_tcp_packet_len);

  private:
    enum TransferMode
    {
        UNKNOWN,
        DEFAULT,
        CHUNKED
    };

    struct HttpStreamState
    {
        TransferMode transfer_mode = TransferMode::UNKNOWN;
        int body_lenght = -1;
        bool headers_read = false;
        char *headers_end_pointer = nullptr;
        HttpVersion http_version = HttpVersion::UNKNOWN;
        std::unordered_map<std::string, std::string> headers;
    };

    template <typename T> struct ParseResult
    {
        bool error = false;
        std::string error_message;
        std::string error_details;
        bool continue_listening = false;
        T value{};
    };

    using HEADER_LIST = std::unordered_map<std::string, std::string>;
    using VersionParseResult = ParseResult<HttpVersion>;
    using TransferModeInfo = ParseResult<std::pair<TransferMode, int>>;

    std::unordered_map<uint64_t, HttpStreamState> m_client_states;

    std::mutex m_mtx;

    bool m_assemble_chunked_requests;

    void resetState(HttpStreamState &state) const;

    VersionParseResult parseHttpVersion(const char *buffer, int buffer_len, int max_buffer_len) const;

    ParseResult<HEADER_LIST> parseHttpHeaders(const char *buffer, int buffer_len, int max_buffer_len,
                                              char *header_end) const;

    // Pre: headers should be read before calling this
    TransferModeInfo getTransferMode(const HEADER_LIST &headers) const;
};

} // namespace pulse::net
