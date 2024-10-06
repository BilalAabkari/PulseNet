#include "PulseNet.h"
#include "utils/ConfigParser.h"
#include "database/DBManager.h"
#include "utils/Logger.h"
#include "networking/NetworkManager.h"
#include <locale>
#include <iostream>
#include <codecvt>
#include <thread>

void handleRequest(char request[], int size) {
  std::cout << request << std::endl;
}

int main() {
  Logger &logger = Logger::getInstance();
  std::locale::global(std::locale("en_US.UTF-8"));
  std::cout.imbue(std::locale());
  std::cerr.imbue(std::locale());

  /********** Read config file ***********/
  ConfigParser parser;
  parser.read();
  std::cout << parser;

  /********** Connect and initialize database ***********/
  DBManager db_manager(parser.getDatabaseConfig());
  try {
    db_manager.connectToDb();
    db_manager.init();
  } catch (const std::exception &ex) {
    logger.log(LogType::DATABASE, LogSeverity::LOG_ERROR, ex.what());
  }

  if (db_manager.isConnected()) {
    logger.log(LogType::DATABASE, LogSeverity::LOG_INFO,
               "Connected to database successfully");
  }

  /********** Initialize sockets ***********/
  NetworkManager requestsListener(80, "127.0.0.1");

  try {
    requestsListener.setupSocket();
    requestsListener.startListening(handleRequest);
  } catch (const std::exception &ex) {
    logger.log(LogType::NETWORK, LogSeverity::LOG_ERROR, ex.what());
  }
  logger.log(LogType::NETWORK, LogSeverity::LOG_INFO,
             "Socket listener created successfully at " +
                 requestsListener.getIp() + ":" +
                 std::to_string(requestsListener.getPort()));

  while (true) {
    // Do something
  }

  return 0;
}
