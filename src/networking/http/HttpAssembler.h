#pragma once
#include "../LoggerManager.h"
#include "../TCPMessageAssembler.h"
#include "../constants.h"
#include "HttpHelpers.h"
#include "HttpMessage.h"
#include <mutex>
#include <string>
#include <unordered_map>

namespace pulse::net
{

class HttpAssembler : public TCPMessageAssembler<HttpMessage>
{
  public:
    HttpAssembler(bool assemble_chunked_requests = true);

    virtual AssemblingResult feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                  int last_tcp_packet_len);

    void setMaxRequestLineLength(int length);
    void setMaxRequestHeaderBytes(int length);
    void setMaxBodySize(int length);

    void enableLogs();
    void disableLogs();

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
        STATE_PARSE_CHUNK_SIZE,
        STATE_DONE,
        STATE_ERROR
    };

    struct HttpStreamState
    {
        int pos = 0;
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

        int length_counter = 0, total_headers_counter = 0;
    };

    std::unordered_map<uint64_t, HttpStreamState> m_client_states;
    std::mutex m_mtx;
    bool m_assemble_chunked_requests;

    void resetState(HttpStreamState &state) const;
    HttpMethod parseMethod(std::string_view part) const;
    bool parseNumber(const std::string &s, int &result) const;
    void log(std::string_view severity, std::string_view message);

    int m_max_request_line_lenght = 4096;
    int m_max_total_headers = 8192;
    int m_max_body_size = 1E6;

    bool m_logs_enabled = false;
};

} // namespace pulse::net
