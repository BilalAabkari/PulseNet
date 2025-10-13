#include "PulseNet.h"
#include "networking/NetworkManager.h"
#include "utils/ConfigParser.h"
#include "utils/Logger.h"
#include <codecvt>
#include <iostream>
#include <locale>
#include <thread>

void handleRequest(char request[], int size)
{
    std::cout << request << std::endl;
}

const std::string YELLOW = "\033[33m"; // Yellow text
const std::string RESET = "\033[0m";   // Reset to default color

int main()
{
    Logger &logger = Logger::getInstance();

    try
    {
        std::locale loc("");
        std::locale::global(loc);
    }
    catch (...)
    {
        std::locale::global(std::locale("C"));
    }

    std::cout.imbue(std::locale());
    std::cerr.imbue(std::locale());

    /********** Read config file ***********/
    ConfigParser parser;
    parser.read();

    /********** Initialize sockets ***********/
    NetworkManager requestsListener(80, "127.0.0.1");

    try
    {
        requestsListener.setupSocket();
        requestsListener.startListening(handleRequest);
    }
    catch (const std::exception &ex)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, ex.what());
        std::cerr << ex.what() << std::endl;
        return -1;
    }
    logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
               "Socket listener created successfully at " + requestsListener.getIp() + ":" +
                   std::to_string(requestsListener.getPort()));

    while (true)
    {
        // Do something
    }

    return 0;
}
