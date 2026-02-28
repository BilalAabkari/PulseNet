#include "Commands.h"
#include "networking/Client.h"
#include "networking/LoggerManager.h"
#include "networking/TCPServer.h"
#include "networking/ThreadPool.h"
#include "networking/http/HttpAssembler.h"
#include "utils/ConfigParser.h"
#include "utils/Console.h"
#include "utils/Logger.h"
#include <codecvt>
#include <iostream>
#include <locale>
#include <thread>

int main()
{
    Logger &logger = Logger::getInstance();

    /********** Read config file ***********/
    ConfigParser parser;
    parser.read();

    /********** Initialize sockets ***********/
    auto assembler = std::make_unique<pulse::net::HttpAssembler>();
    pulse::net::TCPServer<pulse::net::HttpAssembler> server(80, "0.0.0.0", 8, std::move(assembler));

    pulse::net::LoggerManager::setLevel(pulse::net::SEVERITY::TRACE);

    try
    {
        server.start();
    }
    catch (const std::exception &ex)
    {
        logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, ex.what());
        std::cerr << ex.what() << std::endl;
        return -1;
    }

    logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
               "Socket listener created successfully at " + server.getIp() + ":" + std::to_string(server.getPort()));

    pulse::utils::Console console;
    registerCommands(console, server);

    pulse::net::ThreadPool test(4, [&server]() {
        auto request = server.next();

        pulse::net::HttpMessage message = request->message;

        static const std::string RESPONSE = "HTTP/1.1 200 OK\r\n"
                                            "Content-Type: application/json\r\n"
                                            "Content-Length: 35\r\n"
                                            "\r\n"
                                            "{\n\"message\" : \"Message received!\"\n}";

        bool isChunked = message.headerContainsValue("transfer-encoding", "chunked");

        if ((isChunked && message.rawBody().size() == 0) || !isChunked)
        {
            server.send(request->client.id, RESPONSE);
        }
    });

    test.run();
    console.run();

    return 0;
}
