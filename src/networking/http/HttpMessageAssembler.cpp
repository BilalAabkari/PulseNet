#include "HttpMessageAssembler.h"
#include "../constants.h"
#include <algorithm>
#include <charconv>

namespace pulse::net
{

HttpMessageAssembler::HttpMessageAssembler(bool assemble_chunked_requests)
    : m_assemble_chunked_requests(assemble_chunked_requests)
{
}

std::vector<std::string> HttpMessageAssembler::feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                                    int last_tcp_packet_len)
{

    std::lock_guard lock(m_mtx);

    HttpStreamState &state = m_client_states[id];

    std::vector<std::string> messages;

    bool finished = false;

    bool error = false;
    std::string error_message;
    std::string error_details;

    while (!finished && !error && buffer_len > 0)
    {
        if (state.http_version == HttpVersion::UNKNOWN)
        {
            ParseResult<HttpVersion> result = parseHttpVersion(buffer, buffer_len, max_buffer_len);

            if (result.error)
            {
                buffer_len = 0;
                error = true;
                error_message = std::move(result.error_message);
                error_details = std::move(result.error_details);
            }
            else if (result.continue_listening) // Case when we need to wait for more info
            {
                finished = true;
            }
            else
            {
                state.http_version = result.value;
            }
        }
        // HTTP 1.0 ends when the connection closes (the last tcp packet received 0 bytes)
        else if (state.http_version == HttpVersion::HTTP_1_0)
        {
            if (last_tcp_packet_len == 0)
            {
                std::string message(buffer, buffer_len);
                messages.push_back(std::move(message));
                buffer_len = 0;
                finished = true;

                resetState(state);
            }
        }
        else if (state.http_version == HttpVersion::HTTP_1_1)
        {
            if (!state.headers_read)
            {
                char *headers_end = nullptr;

                ParseResult<HEADER_LIST> result = parseHttpHeaders(buffer, buffer_len, max_buffer_len, headers_end);

                if (result.error)
                {
                    buffer_len = 0;
                    error = true;
                    error_message = std::move(result.error_message);
                    error_details = std::move(result.error_details);
                }
                else if (result.continue_listening) // Case when we need to wait for more info
                {
                    finished = true;
                }
                else
                {
                    state.headers = result.value;
                    state.headers_end_pointer = headers_end;
                }
            }
            else
            {
                if (state.transfer_mode == TransferMode::UNKNOWN)
                {
                    TransferModeInfo result = getTransferMode(state.headers);
                    if (result.error)
                    {
                        buffer_len = 0;
                        error = true;
                        error_message = std::move(result.error_message);
                        error_details = std::move(result.error_details);
                    }
                    else
                    {
                        state.transfer_mode = result.value.first;
                        state.body_lenght = result.value.second;
                    }
                }
                else if (state.transfer_mode == TransferMode::DEFAULT)
                {
                    ptrdiff_t current_body_lenght = (buffer + buffer_len) - (state.headers_end_pointer + 4);
                    if (current_body_lenght < state.body_lenght)
                    {
                        // Keep listening
                        finished = true;
                    }
                    else
                    {
                        // The entire message is in the buffer, so we take it and shift the buffer contents to the
                        // beggining.
                        std::string message;
                        message.reserve(state.body_lenght + 50);

                        char *body_start_ptr = state.headers_end_pointer + 4;

                        // Copy headers
                        message.append(buffer, body_start_ptr);
                        message.append(body_start_ptr, body_start_ptr + state.body_lenght);

                        messages.push_back(std::move(message));

                        char *next_message_ptr = body_start_ptr + state.body_lenght;

                        int new_lenght = 0;
                        if (next_message_ptr < buffer + buffer_len)
                        {
                            ptrdiff_t len = buffer + buffer_len - next_message_ptr;
                            new_lenght = len;
                            std::memmove(buffer, buffer, new_lenght);
                        }

                        buffer_len = new_lenght;

                        resetState(state);
                    }
                }
                else if (state.transfer_mode == TransferMode::CHUNKED)
                {
                    // Read all present chunks
                    char *ptr = state.headers_end_pointer + 4;
                    char *end = buffer + buffer_len;

                    if (ptr >= end && buffer_len == max_buffer_len)
                    {
                        error = true;
                        error_message = "Could not parse http message. Malformed request.";
                        error_details = "Cound not find the chunk size.";
                        buffer_len = 0;
                    }
                    else if (ptr >= end)
                    {
                        // Keep listening
                        finished = true;
                    }
                    else
                    {
                        bool last_chunk_read = false;
                        std::string body;

                        // Read chunks
                        while (ptr < end && !last_chunk_read && !error && !finished)
                        {
                            // Find next break line to get the chunk size
                            const char *pattern = "\r\n";
                            char *next_break_line = std::search(ptr, end, pattern, pattern + sizeof(pattern));

                            if (next_break_line == end)
                            {
                                error = true;
                                error_message = "Could not parse http message. Malformed request.";
                                error_details = "Cound not find the chunk size.";
                                buffer_len = 0;
                            }
                            else
                            {
                                // Parse the size
                                int size;
                                auto [_, ec] = std::from_chars(ptr, next_break_line, size, 16);
                                ptr = next_break_line + 2;

                                if (ec != std::errc())
                                {
                                    error = true;
                                    error_message = "Could not parse http message. Malformed request.";
                                    error_details = "Cound not parse the chunk size.";
                                    buffer_len = 0;
                                }
                                else if (size == 0)
                                {
                                    // It means it's the last chunk, so we stop
                                    last_chunk_read = true;
                                }
                                else if (ptr + size >= end && buffer_len == max_buffer_len)
                                {
                                    error = true;
                                    error_message = "Could not parse http message. Malformed request.";
                                    error_details = "Chunk has not enough data for the specified size.";
                                    buffer_len = 0;
                                }
                                else if (ptr + size >= end)
                                {
                                    // keep listening
                                    finished = true;
                                }
                                else
                                {
                                    body.append(ptr, size);
                                }
                            }
                        }

                        if (!error)
                        {
                            if (m_assemble_chunked_requests && !last_chunk_read)
                            {
                                finished = true;
                            }
                            else
                            {
                                std::string message;

                                message.append(buffer, state.headers_end_pointer + 4);
                                message.append(body);
                                messages.push_back(message);
                            }

                            // If m_assemble_chunked_requests is false and we did not read the last chunk, we keep the
                            // headers for the next chunks. Ohterwise, we can clear them:
                            if (!m_assemble_chunked_requests && !last_chunk_read)
                            {
                                char *start_of_next = state.headers_end_pointer + 4 + body.size();
                                size_t remaining_len = buffer_len - (start_of_next - buffer);

                                if (remaining_len > 0)
                                {
                                    memmove(state.headers_end_pointer + 4, start_of_next, remaining_len);
                                }
                            }
                            else if (last_chunk_read)
                            {
                                char *start_of_next = state.headers_end_pointer + 4 + body.size();
                                size_t len = buffer + buffer_len - start_of_next;

                                if (len > 0)
                                {
                                    memmove(buffer, start_of_next, len);
                                    buffer_len = len;
                                }
                                else
                                {
                                    buffer_len = 0;
                                }

                                resetState(state);
                            }
                        }
                    }
                }
            }
        }
    }
}

HttpMessageAssembler::VersionParseResult HttpMessageAssembler::parseHttpVersion(const char *buffer, int buffer_len,
                                                                                int max_buffer_len) const
{
    VersionParseResult result;
    result.continue_listening = false;

    // If we still don't know the http version, we search it in the buffer.
    const char *pattern = "\r\n";
    const char *it = std::search(buffer, buffer + buffer_len, pattern, pattern + sizeof(pattern));

    // If not found and we already have the buffer filled to max, we throw away
    // the entire buffer
    if (it == buffer + buffer_len && buffer_len == max_buffer_len)
    {
        result.error = true;
        result.error_message = "Could not parse http message. Malformed request.";
        result.error_details = "Could not parse HTTP version.";
    }
    else if (it == buffer + buffer_len)
    {
        result.continue_listening = true;
    }
    else
    {
        const char *pattern_http = "HTTP/";
        const char *it_http = std::search(buffer, it, pattern_http, pattern_http + std::strlen(pattern_http));

        // If not found, throw it away
        if (it_http == it)
        {
            buffer_len = 0;
            result.error = true;
            result.error_message = "Could not parse http message. Malformed request.";
            result.error_details = "Could not parse HTTP version.";
        }
        else
        {
            // The next 3 chars are the version
            std::string version = "---";

            for (int i = 0; i < 3; i++)
            {
                version[i] = *(it_http + i);
            }

            if (version == "1.0")
            {
                result.value = HttpVersion::HTTP_1_0;
            }
            else if (version == "1.1")
            {
                result.value = HttpVersion::HTTP_1_1;
            }
            else
            {
                buffer_len = 0;
                result.error = true;
                result.error_message = "Could not parse http message. Malformed request.";
                result.error_details = "Http version \"" + version + "\" is not valid.";
            }
        }
    }

    return result;
}

HttpMessageAssembler::ParseResult<HttpMessageAssembler::HEADER_LIST> HttpMessageAssembler::parseHttpHeaders(
    const char *buffer, int buffer_len, int max_buffer_len, char *header_end) const
{

    ParseResult<HEADER_LIST> result;
    result.continue_listening = false;

    const char *pattern = "\r\n\r\n";
    const char *headers_end = std::search(buffer, buffer + buffer_len, pattern, pattern + std::strlen(pattern));

    // We search for the end of the headers. If not found and the buffer is out of
    // space, we throw error
    if (headers_end == buffer + buffer_len && buffer_len == max_buffer_len)
    {
        result.error = true;
        result.error_message = "Could not parse http message. Malformed request.";
        result.error_details = "Error parsing headers";
    }
    else if (headers_end == buffer + buffer_len)
    {
        // Wait for more data
        result.continue_listening = true;
    }
    else
    {
        // Read the headers and save them
        const char *ptr = buffer;

        std::string current_header = "";
        current_header.reserve(100);

        while (ptr != headers_end - 2)
        {
            if (*ptr == '\r' && *(ptr + 1) == '\n')
            {
                size_t separator = current_header.find(':');
                if (separator != std::string::npos)
                {
                    std::string name = current_header.substr(0, separator);
                    std::string value = current_header.substr(separator + 1);

                    std::transform(name.begin(), name.end(), name.begin(),
                                   [](unsigned char c) { return std::tolower(c); });

                    auto it = result.value.find(name);
                    if (it != result.value.end())
                    {
                        result.value[name] = it->second + "," + value;
                    }
                    else
                    {
                        result.value[name] = value;
                    }
                }
                ptr = ptr + 2;
                current_header = "";
            }
            else
            {
                current_header.push_back(*ptr);
                ptr++;
            }
        }

        header_end = const_cast<char *>(headers_end);
    }

    return result;
}

HttpMessageAssembler::TransferModeInfo HttpMessageAssembler::getTransferMode(const HEADER_LIST &headers) const
{
    TransferModeInfo result;

    // In HTTP 1.1, headers must have Transfer-Encoding "chunked" or
    // Content-lenght, one of them If both present, use chunked.
    bool chunked = false;
    int content_lenght = -1;

    auto it = headers.find("transfer-encoding");
    if (it != headers.end())
    {
        if (it->second.find("chunked") != std::string::npos)
        {
            chunked = true;
        }
    }

    if (!chunked)
    {
        auto it = headers.find("content-length");
        if (it != headers.end())
        {
            int num;
            auto [ptr, ec] = std::from_chars(it->second.data(), it->second.data() + it->second.size(), num);
            if (ec == std::errc())
            {
                content_lenght = num;
            }
        }
    }

    if (!chunked && content_lenght == -1)
    {
        result.error = true;
        result.error_message = "Could not parse http message. Malformed request.";
        result.error_details = "In http 1.1, either Content-Length or "
                               "Transfer-encoding: chunked should be "
                               "present";
    }
    else if (chunked)
    {
        result.value.first = TransferMode::CHUNKED;
        result.value.second = -1;
    }
    else
    {
        result.value.first = TransferMode::DEFAULT;
        result.value.second = content_lenght;
    }

    return result;
}

void HttpMessageAssembler::resetState(HttpStreamState &state) const
{
    state.transfer_mode = TransferMode::UNKNOWN;
    state.body_lenght = -1;
    state.headers_read = false;
    state.headers_end_pointer = nullptr;
    state.http_version = HttpVersion::UNKNOWN;
    state.headers.clear();
}

} // namespace pulse::net
