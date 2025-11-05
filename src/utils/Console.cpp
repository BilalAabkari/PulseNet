#include "Console.h"

namespace pulse::utils
{

Console::Console()
{
}

void Console::showPrompt(std::ostream &os) const
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_s(&local_tm, &now_time);

    // ANSI color codes
    const char *RESET = "\033[0m";
    const char *BLUE = "\033[34m";
    const char *CYAN = "\033[36m";
    const char *GREEN = "\033[32m";
    const char *YELLOW = "\033[33m";

    os << YELLOW << "_(" << GREEN << "identifier" << YELLOW << ")-(" << CYAN << std::setw(2) << std::setfill('0')
       << local_tm.tm_hour << ":" << std::setw(2) << std::setfill('0') << local_tm.tm_min << ":" << std::setw(2)
       << std::setfill('0') << local_tm.tm_sec << YELLOW << ")" << RESET << "\n";

    os << YELLOW << "L " << RESET;
}

void Console::handle(std::ostream &os, std::string command, std::vector<std::string> &args) const
{
    auto it = m_command_list.find(command);
    if (it != m_command_list.end())
    {
        it->second(args);
    }
    else
    {
        os << "[ERROR] Command not found/n";
    }
}

void Console::registerCommand(std::string command, CommandHandler handler)
{
    m_command_list[command] = handler;
}

void Console::run() const
{
    std::string line;
    while (true)
    {
        showPrompt(std::cout);
        if (!std::getline(std::cin, line))
            break;

        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty())
            continue;

        auto it = m_command_list.find(tokens[0]);
        if (it != m_command_list.end())
        {
            it->second(tokens);
        }
        else
        {
            std::cerr << "Unknown command: " << tokens[0] << "\n";
        }
    }
}

std::vector<std::string> Console::tokenize(const std::string &line) const
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

} // namespace pulse::utils
