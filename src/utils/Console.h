#pragma once

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace pulse::utils
{
class Console
{

  public:
    using CommandHandler = std::function<void(const std::vector<std::string> &args)>;

    Console();

    void showPrompt(std::ostream &os) const;

    void handle(std::ostream &os, std::string command, std::vector<std::string> &args) const;

    void registerCommand(std::string command, CommandHandler handler);

    void run() const;

  private:
    std::vector<std::string> tokenize(const std::string &line) const;

    std::unordered_map<std::string, CommandHandler> m_command_list;
};
} // namespace pulse::utils
