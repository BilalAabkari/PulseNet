#include "HttpMessageAssembler.h"
#include "../../utils/Logger.h"
#include "../constants.h"
#include <algorithm>
#include <charconv>

namespace pulse::net
{

HttpMessageAssembler::HttpMessageAssembler()
{
}

std::vector<std::string> HttpMessageAssembler::feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                                    int last_tcp_packet_len)
{
    Logger &logger = Logger::getInstance();

    std::lock_guard lock(m_mtx);

    HttpStreamState &state = m_client_states[id];

    std::vector<std::string> messages;

    bool finished = false;

    bool error = false;
    std::string error_message;
    std::string error_details;

    while (!finished && !error)
    {
        if (state.http_version == HttpVersion::UNKNOWN)
        {
            // If we still don't know the http version, we search it in the buffer.
            char pattern[] = {'\r', '\n'};
            char *it = std::search(buffer, buffer + buffer_len, pattern, pattern + sizeof(pattern));

            // If not found and we already have the buffer filled to max, we throw away the entire buffer
            if (it == buffer + buffer_len && buffer_len == max_buffer_len)
            {
                buffer_len = 0;
                error = true;
                error_message = "Could not parse http message. Malformed request.";
                error_details = "Could not parse HTTP version.";
            }
            else
            {
                const char *pattern_http = "HTTP/";
                char *it_http = std::search(buffer, it, pattern_http, pattern_http + std::strlen(pattern_http));

                // If not found, throw it away
                if (it_http == it)
                {
                    buffer_len = 0;
                    error = true;
                    error_message = "Could not parse http message. Malformed request.";
                    error_details = "Could not parse HTTP version.";
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
                        state.http_version = HttpVersion::HTTP_1_0;
                    }
                    else if (version == "1.1")
                    {
                        state.http_version = HttpVersion::HTTP_1_1;
                    }
                    else
                    {
                        buffer_len = 0;
                        error = true;
                        error_message = "Could not parse http message. Malformed request.";
                        error_details = "Http version \"" + version + "\" is not valid.";
                    }
                }
            }
        }

        // HTTP 1.0 ends when the connection closes (the last tcp packet received 0 bytes)
        if (state.http_version == HttpVersion::HTTP_1_0)
        {
            if (last_tcp_packet_len == 0)
            {
                std::string message(buffer, buffer_len);
                messages.push_back(std::move(message));
                buffer_len = 0;
                finished = true;

                state.body_lenght = -1;
                state.headers_read = false;
                state.headers.clear();
                state.http_version = HttpVersion::UNKNOWN;
            }
        }

        if (state.http_version == HttpVersion::HTTP_1_1)
        {
            if (!state.headers_read)
            {

                const char *pattern = "\r\n\r\n";
                char *headers_end = std::search(buffer, buffer + buffer_len, pattern, pattern + std::strlen(pattern));

                // We search for the end of the headers. If not found and the buffer is out of space, we throw away
                // everything.
                if (headers_end == buffer + buffer_len && buffer_len == max_buffer_len)
                {
                    buffer_len = 0;
                    error = true;
                    error_message = "Could not parse http message. Malformed request.";
                    error_details = "Error parsing headrs";
                }
                else if (headers_end == buffer + buffer_len)
                {
                    // keep reading
                    finished = true;
                }
                else
                {
                    // Read the headers and save them
                    char *ptr = buffer;

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

                                auto it = state.headers.find(name);
                                if (it != state.headers.end())
                                {
                                    state.headers[name] = it->second + "," + value;
                                }
                                else
                                {
                                    state.headers[name] = value;
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

                    state.headers_read = true;
                }
            }

            if (state.headers_read)
            {
                // In HTTP 1.1, headers must have Transfer-Encoding "chunked" or Content-lenght, one of them
                // If both present, use chunked.
                bool chunked = false;
                int content_lenght = -1;

                auto it = state.headers.find("Transfer-Encoding");
                if (it != state.headers.end())
                {
                    if (it->second.find("chunked") != std::string::npos)
                    {
                        chunked = true;
                    }
                }

                if (!chunked)
                {
                    auto it = state.headers.find("Content-Length");
                    if (it != state.headers.end())
                    {
                        int num;
                        auto [ptr, ec] = std::from_chars(it->second.data(), it->second.data() + it->second.size(), num);
                        if (ec != std::errc())
                        {
                            content_lenght = num;
                        }
                    }
                }

                if (!chunked && content_lenght == -1)
                {
                    // Error!
                }
            }
        }
    }
}

} // namespace pulse::net
