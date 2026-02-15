#pragma once
#include <algorithm>
#include <charconv>
#include <string>

namespace pulse::net
{

class Utils
{
  public:
    inline static std::string toLowerAscii(const std::string_view &s)
    {
        std::string result(s);

        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            if (c >= 'A' && c <= 'Z')
                return (unsigned char)(c + 32);
            return c;
        });
        return result;
    }

    inline static bool containsToken(const std::string &value, const std::string &token)
    {
        int i = 0;
        int i_start = 0;

        bool found = false;
        while (!found && i < value.size())
        {
            bool quoted = false;

            while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
                i++;

            i_start = i;

            if (value[i] == '"')
            {
                quoted = true;
                while (i < value.size() - 1)
                {
                    if (value[i] != '\\' && value[i + 1] == '"')
                        break;
                    i++;
                }

                if (i >= value.size())
                {
                    // Not found the next quote, which means maformed value
                    break;
                }

                i++; // Skip the quote
                i_start++;
            }
            else
            {
                while (i < value.size() && value[i] != ',')
                    i++;
            }

            int i_end = i;

            while (i_end > 0 && (value[i_end - 1] == ' ' || value[i_end - 1] == '\t'))
                i_end--;

            if (i_end - i_start == token.size())
            {
                int j = 0;

                while (j < token.size() && token[j] == value[i_start])
                {
                    j++;
                    i_start++;
                }

                if (j == token.size())
                {
                    found = true;
                    break;
                }
            }

            i++;

            if (quoted)
            {
                while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
                    i++;

                if (i < value.size() && value[i] == ',')
                {
                    i++;
                }
                else
                {
                    break; // Malformed
                }
            }
        }

        return found;
    }

    static bool parseHexadecimal(const std::string_view &s, int &result)
    {

        const char *first = s.data();
        const char *last = first + s.size();

        auto [ptr, ec] = std::from_chars(first, last, result, 16);

        return ec == std::errc{};
    }
};

} // namespace pulse::net
