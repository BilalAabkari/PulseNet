#include "HttpAssembler.h"

#include "../LoggerManager.h"
#include "../Utils.h"
#include <algorithm>
#include <charconv>

namespace pulse::net
{

HttpAssembler::HttpAssembler(bool assemble_chunked_requests) : m_assemble_chunked_requests(assemble_chunked_requests)
{
}

HttpAssembler::AssemblingResult HttpAssembler::feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                                    int last_tcp_packet_len)
{

    std::lock_guard lock(m_mtx);

    HttpStreamState &client_state = m_client_states[id];

    bool finished = false;

    std::string error_message = "Error: malformed request syntax";
    std::string error_details;

    HttpAssembler::AssemblingResult result;

    while (client_state.pos < buffer_len)
    {
        int i = client_state.pos;

        char c = buffer[i];
        client_state.length_counter++;

        switch (client_state.state)
        {
        case HttpState::STATE_PARSE_RESPONSE_OR_REQUEST:
            if (c == ' ' && client_state.i_end - client_state.i_start > 0)
            {
                std::string_view part(buffer + client_state.i_start, client_state.i_end - client_state.i_start);
                if (part == "HTTP/1.0")
                {
                    client_state.http_version = HttpVersion::HTTP_1_0;
                    client_state.state = HttpState::STATE_PARSE_RESPONSE_STATUS;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                    client_state.type = HttpType::RESPONSE;
                }
                else if (part == "HTTP/1.1")
                {
                    client_state.http_version = HttpVersion::HTTP_1_1;
                    client_state.state = HttpState::STATE_PARSE_RESPONSE_STATUS;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                    client_state.type = HttpType::RESPONSE;
                }
                else
                {
                    client_state.method = parseMethod(part);
                    if (client_state.method == HttpMethod::INVALID)
                    {
                        client_state.state = HttpState::STATE_ERROR;
                        log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                        ": Invalid first line. Expected HTTP version or a method.");
                    }
                    else
                    {
                        client_state.type = HttpType::REQUEST;
                        client_state.state = HttpState::STATE_PARSE_REQUEST_URI;
                        client_state.i_start = i + 1;
                        client_state.i_end = i + 1;
                    }
                }
            }
            else if (client_state.length_counter > 8)
            {
                log("WARN",
                    "Rejected http request of connection " + std::to_string(id) + ": Invalid first line. Too long");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;

        case HttpState::STATE_PARSE_RESPONSE_STATUS:
            if (c == ' ')
            {
                int length = client_state.i_end - client_state.i_start;
                if (length != 3 || !isdigit(buffer[client_state.i_start]) ||
                    !isdigit(buffer[client_state.i_start + 1]) || !isdigit(buffer[client_state.i_start + 1]))
                {
                    client_state.state = HttpState::STATE_ERROR;
                }
                else
                {
                    client_state.http_code = (buffer[0] - '0') * 100 + (buffer[1] - '0') * 10 + (buffer[2] - '0');
                    client_state.state = HttpState::STATE_PARSE_RESPONSE_HEDAERS_START;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                }
            }
            else if (client_state.length_counter > m_max_request_line_lenght)
            {
                log("WARN",
                    "Rejected http request of connection " + std::to_string(id) + ": Invalid first line. Too long");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (isdigit(static_cast<unsigned char>(c)))
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;

        case HttpState::STATE_PARSE_RESPONSE_HEDAERS_START:
            if (client_state.i_end - client_state.i_start > 0 && c == '\n' && buffer[client_state.i_end] == '\r')
            {
                client_state.length_counter = 0;
                client_state.state = HttpState::STATE_PARSE_HEADER_NAME;
                client_state.i_start = i + 1;
                client_state.i_end = i + 1;
            }
            else if (client_state.length_counter > m_max_request_line_lenght)
            {
                log("WARN",
                    "Rejected http request of connection " + std::to_string(id) + ": Invalid first line. Too long");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;
        case HttpState::STATE_PARSE_REQUEST_URI:
            if (c == ' ' && client_state.i_end - client_state.i_start == 0)
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (c == ' ')
            {
                client_state.uri.assign(buffer + client_state.i_start, client_state.i_end - client_state.i_start);
                client_state.i_start = i + 1;
                client_state.i_end = i + 1;
                client_state.state = HttpState::STATE_PARSE_REQUEST_HTTP_VERSION;
            }
            else if (client_state.length_counter > m_max_request_line_lenght)
            {
                log("WARN",
                    "Rejected http request of connection " + std::to_string(id) + ": Invalid first line. Too long");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;
        case HttpState::STATE_PARSE_REQUEST_HTTP_VERSION:
            if (client_state.i_end - client_state.i_start > 1 && c == '\n' && buffer[client_state.i_end - 1] == '\r')
            {
                std::string_view part(buffer + client_state.i_start, client_state.i_end - 1 - client_state.i_start);
                if (part == "HTTP/1.0")
                {
                    client_state.http_version = HttpVersion::HTTP_1_0;
                    client_state.state = HttpState::STATE_PARSE_HEADER_NAME;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                    client_state.length_counter = 0;
                }
                else if (part == "HTTP/1.1")
                {
                    client_state.http_version = HttpVersion::HTTP_1_1;
                    client_state.state = HttpState::STATE_PARSE_HEADER_NAME;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                    client_state.length_counter = 0;
                }
                else
                {
                    client_state.state = HttpState::STATE_ERROR;
                }
            }
            else if (client_state.length_counter > m_max_request_line_lenght)
            {
                log("WARN",
                    "Rejected http request of connection " + std::to_string(id) + ": Invalid first line. Too long");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;
        case HttpState::STATE_PARSE_HEADER_NAME: {

            client_state.total_headers_counter++;
            int size = client_state.i_end - client_state.i_start;

            if (c == ':' && size > 0)
            {

                while (*(buffer + client_state.i_start) == ' ' && client_state.i_start < client_state.i_end)
                    client_state.i_start++;

                while (*(buffer + client_state.i_end) == ' ' && client_state.i_start < client_state.i_end)
                    client_state.i_end--;

                size = client_state.i_end - client_state.i_start;

                if (size > 0)
                {
                    client_state.header_name_start = client_state.i_start;
                    client_state.header_name_end = client_state.i_end;
                    client_state.state = HttpState::STATE_PARSE_HEADER_VALUE;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                }
                else
                {
                    client_state.state = HttpState::STATE_ERROR;
                }
            }
            else if (c == ':' && size == 0)
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (c == '\n' && buffer[i - 1] == '\r' && client_state.i_end - client_state.i_start == 1)
            {

                auto it = client_state.headers.find("transfer-mode");
                if (it != client_state.headers.end())
                {
                }
                else
                {
                    auto it = client_state.headers.find("content-length");
                    if (it != client_state.headers.end())
                    {
                        int length = 0;
                        if (parseNumber(it->second, length) && length <= m_max_body_size)
                        {
                            client_state.state = HttpState::STATE_PARSE_BODY;
                            client_state.body_lenght = length;
                            client_state.i_start = i + 1;
                            client_state.i_end = i + 1;
                        }
                        else if (length > m_max_body_size)
                        {
                            log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                            ": Invalid headers. Maximum number "
                                            "of bytes for "
                                            "headers exceded");
                            client_state.state = HttpState::STATE_ERROR;
                        }
                        else
                        {
                            log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                            ": Could not parse content length octets");
                            client_state.state = HttpState::STATE_ERROR;
                        }
                    }
                    else
                    {
                        client_state.state = HttpState::STATE_DONE;
                    }
                }
            }
            else if (client_state.total_headers_counter > m_max_total_headers)
            {
                log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                ": Invalid headers. Maximum number of bytes for "
                                "headers exceded");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;
        }
        case HttpState::STATE_PARSE_HEADER_VALUE:
            client_state.total_headers_counter++;
            if (client_state.i_end - client_state.i_start > 0 && c == '\n' && buffer[client_state.i_end - 1] == '\r')
            {
                while (*(buffer + client_state.i_start) == ' ' && client_state.i_start < client_state.i_end)
                    client_state.i_start++;

                while (*(buffer + client_state.i_end) == ' ' && client_state.i_start < client_state.i_end)
                    client_state.i_end--;

                std::string_view header_name(buffer + client_state.header_name_start,
                                             client_state.header_name_end - client_state.header_name_start);

                std::string_view header_value(buffer + client_state.i_start,
                                              client_state.i_end - 1 - client_state.i_start);

                if (header_value.size() == 0)
                {
                    client_state.state = HttpState::STATE_ERROR;
                }
                else
                {
                    // TODO: IF KEY EXISTS, APPEND
                    client_state.headers.emplace(Utils::toLowerAscii(header_name), header_value);
                    client_state.state = HttpState::STATE_PARSE_HEADER_NAME;
                    client_state.i_start = i + 1;
                    client_state.i_end = i + 1;
                }
            }
            else if (client_state.total_headers_counter > m_max_total_headers)
            {
                log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                ": Invalid headers. Maximum number of bytes for "
                                "headers exceded");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (static_cast<unsigned char>(c) <= 127)
            {
                client_state.i_end++;
            }
            else
            {
                client_state.state = HttpState::STATE_ERROR;
            }
            break;
        case HttpState::STATE_PARSE_BODY: {
            client_state.i_end++;
            int size = client_state.i_end - client_state.i_start;
            if (size > m_max_body_size)
            {
                log("WARN", "Rejected http request of connection " + std::to_string(id) +
                                ": Invalid headers. Maximum number of bytes for "
                                "body exceded");
                client_state.state = HttpState::STATE_ERROR;
            }
            else if (size == client_state.body_lenght)
            {
                std::string body(buffer + client_state.i_start, client_state.body_lenght);

                result.messages.emplace_back(client_state.http_version, client_state.method,
                                             std::move(client_state.uri), std::move(client_state.headers),
                                             std::move(body));

                if (client_state.pos == buffer_len - 1)
                {
                    buffer_len = 0;
                }
                else
                {
                    std::memmove(buffer, buffer + client_state.pos, buffer_len - client_state.pos);
                }

                resetState(client_state);
                continue;
            }
            break;
        }
        case HttpState::STATE_DONE:
            break;
        case HttpState::STATE_ERROR:
            break;
        default:
            break;
        }

        if (client_state.state == HttpState::STATE_ERROR)
        {
            break;
        }

        client_state.pos++;
    }

    if (client_state.state == HttpState::STATE_ERROR)
    {
        result.error = true;
        std::ostringstream oss;
        oss << "{\n"
            << "  \"message\": \"" << error_message << "\",\n"
            << "  \"details\": \"" << error_details << "\"\n"
            << "}";

        std::string json_body = oss.str();
        size_t size = json_body.size();

        HttpMessage response(client_state.http_version, HttpStatus::BAD_REQUEST, std::move(json_body));

        response.addHeader("Content-Lenght", std::to_string(size));
        result.error_message = response.serialize();
    }

    return result;
}

void HttpAssembler::setMaxRequestLineLength(int length)
{
    m_max_request_line_lenght = length;
}

void HttpAssembler::setMaxRequestHeaderBytes(int length)
{
    m_max_total_headers = length;
}

void HttpAssembler::setMaxBodySize(int length)
{
    m_max_body_size = length;
}

void HttpAssembler::enableLogs()
{
    m_logs_enabled = true;
}

void HttpAssembler::disableLogs()
{
    m_logs_enabled = false;
}

void HttpAssembler::resetState(HttpStreamState &state) const
{
    state.transfer_mode = TransferMode::UNKNOWN;
    state.body_lenght = -1;
    state.http_version = HttpVersion::UNKNOWN;

    state.headers = std::unordered_map<std::string, std::string>();
    state.state = HttpState::STATE_PARSE_RESPONSE_OR_REQUEST;

    state.method = HttpMethod::UNKNOWN;
    state.type = HttpType::UNKNOWN;
    state.http_code = -1;

    state.uri = "";
    state.i_start = 0;
    state.i_end = 0;
    state.header_name_start = 0;
    state.header_name_end = 0;
    state.header_value_start = 0;
    state.header_value_end = 0;
    state.length_counter = 0;
    state.total_headers_counter = 0;

    state.pos = 0;
}

HttpMethod HttpAssembler::parseMethod(std::string_view part) const
{
    switch (part.size())
    {
    case 3:
        if (part == "GET")
            return HttpMethod::GET;
        if (part == "PUT")
            return HttpMethod::PUT;
        break;

    case 4:
        if (part == "POST")
            return HttpMethod::POST;
        if (part == "HEAD")
            return HttpMethod::HEAD;
        break;

    case 5:
        if (part == "PATCH")
            return HttpMethod::PATCH;
        if (part == "TRACE")
            return HttpMethod::TRACE;
        break;

    case 6:
        if (part == "DELETE")
            return HttpMethod::HTTP_DELETE;
        break;

    case 7:
        if (part == "OPTIONS")
            return HttpMethod::OPTIONS;
        if (part == "CONNECT")
            return HttpMethod::CONNECT;
        break;
    }

    return HttpMethod::INVALID;
}

bool HttpAssembler::parseNumber(const std::string &s, int &result) const
{
    bool is_quoted = s.size() > 2 && s.front() == '"' && s[s.size() - 1] == '"';

    const char *first = s.data();
    const char *last = first + s.size();

    if (is_quoted)
    {
        first++;
        last--;
    }

    auto [ptr, ec] = std::from_chars(first, last, result);

    return ec == std::errc{};
}

void HttpAssembler::log(std::string_view severity, std::string_view message)
{
    if (m_logs_enabled)
        LoggerManager::get_logger()->write(severity, message);
}

} // namespace pulse::net
