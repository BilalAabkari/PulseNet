#pragma once
#include <iostream>
#include <string_view>

namespace pulse::net
{

class LoggerInterface
{
  public:
    virtual ~LoggerInterface() = default;

    virtual void write(std::string_view severity, std::string_view &message)
    {
        std::cout << "[" << severity << "] " << message << "\n";
    };
};

}; // namespace pulse::net
