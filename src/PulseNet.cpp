#include "PulseNet.h"
#include "Commands.h"
#include "networking/Client.h"
#include "networking/NetworkManager.h"
#include "networking/ThreadPool.h"
#include "networking/http/HttpAssembler.h"
#include "utils/ConfigParser.h"
#include "utils/Console.h"
#include "utils/Logger.h"
#include <codecvt>
#include <iostream>
#include <locale>
#include <thread>

void handleRequest(pulse::net::Client &client, char request[])
{
    std::cout << request << std::endl;
}

int main()
{
    Logger &logger = Logger::getInstance();

    /********** Read config file ***********/
    ConfigParser parser;
    parser.read();

    /********** Initialize sockets ***********/
    std::unique_ptr<pulse::net::HttpAssembler> assembler = std::make_unique<pulse::net::HttpAssembler>();
    assembler->enableLogs();

    pulse::net::NetworkManager<pulse::net::HttpAssembler> server(80, "127.0.0.1", 2, std::move(assembler));

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

    pulse::net::ThreadPool test(1, [&server]() {
        auto request = server.next();
        pulse::net::HttpMessage message = request->message;

        std::cout << "*********************************************************"
                     "********\n";
        std::cout << "CLIENT ID: " << std::to_string(request->client.id)
                  << "\nIP ADDRESS: " << request->client.ip_address
                  << "\nPORT: " << std::to_string(request->client.port) << "\n\n";
        std::cout << message.serialize();

        std::ostringstream response_body;
        response_body << "{\n"
                      << "\"message\" : \"Message received!\"\n"
                      << "}";

        pulse::net::HttpMessage response(pulse::net::HttpVersion::HTTP_1_1, pulse::net::HttpStatus::OK,
                                         response_body.str());
        response.addHeader("Content-Type", "application/json");

        server.send(request->client.id, response.serialize());
    });

    test.run();

    console.run();

    return 0;
}
