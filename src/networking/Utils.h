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
                return (unsigned char)(c + ('a' - 'A'));
            return c;
        });
        return result;
    }
};

} // namespace pulse::net
