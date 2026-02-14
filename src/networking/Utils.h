#pragma once
#include <algorithm>
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

            while (i < value.size() && (value[i] == ' ' || value[i] == '\t'))
                i++;

            i_start = i;

            while (i < value.size() && value[i] != ',')
                i++;

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
        }

        return found;
    }
};

} // namespace pulse::net
