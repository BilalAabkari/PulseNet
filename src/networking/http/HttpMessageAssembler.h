#pragma once
#include "../TCPMessageAssembler.h"
#include "../constants.h"
#include "HttpHelpers.h"
#include "HttpResponse.h"
#include <mutex>
#include <unordered_map>

namespace pulse::net
{

class HttpMessageAssembler : public TCPMessageAssembler
{
  public:
    HttpMessageAssembler(bool assemble_chunked_requests = true);

    virtual AssemblingResult feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len);

  private:
    enum class TransferMode
    {
        UNKNOWN,
        DEFAULT,
        CHUNKED
    };

    enum HttpState
    {
        STATE_START,
        STATE_PARSE_RESPONSE_OR_REQUEST,
        STATE_PARSE_RESPONSE_STATUS,
        STATE_PARSE_REQUEST_URI,
        STATE_PARSE_RESPONSE_HEDAERS_START,
        STATE_PARSE_REQUEST_HTTP_VERSION,
        STATE_PARSE_HEADER_NAME,
        STATE_PARSE_HEADER_VALUE,
        STATE_PARSE_BODY,
        STATE_ERROR
    };

    enum class HttpType
    {
        REQUEST,
        RESPONSE,
        UNKNOWN
    };

    enum class HttpMethod
    {
        GET,
        POST,
        PUT,
        HTTP_DELETE,
        PATCH,
        TRACE,
        HEAD,
        OPTIONS,
        CONNECT,
        INVALID,
        UNKNOWN
    };

    struct HttpStreamState
    {
        TransferMode transfer_mode = TransferMode::UNKNOWN;
        int body_lenght = -1;
        HttpVersion http_version = HttpVersion::UNKNOWN;

        std::unordered_map<std::string, std::string> headers;

        HttpState state = HttpState::STATE_PARSE_RESPONSE_OR_REQUEST;
        HttpMethod method = HttpMethod::UNKNOWN;
        HttpType type = HttpType::UNKNOWN;
        int http_code = -1;

        std::string uri;

        int i_start = 0, i_end = 0;
        int header_name_start = 0, header_name_end = 0;
        int header_value_start = 0, header_value_end = 0;
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

    HttpMethod parse_method(std::string_view part) const;
};

} // namespace pulse::net
