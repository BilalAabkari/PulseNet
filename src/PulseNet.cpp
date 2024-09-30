#include "PulseNet.h"
#include "utils/ConfigParser.h"
#include "database/DBManager.h"
#include "networking/NetworkManager.h"
#include <locale>
#include <iostream>
#include <codecvt>

int main() {
  std::locale::global(std::locale("en_US.UTF-8"));
  std::cout.imbue(std::locale());
  std::cerr.imbue(std::locale());

  ConfigParser parser;
  parser.read();
  std::cout << parser;

  DBManager db_manager(parser.getDatabaseConfig());
  db_manager.connectToDb();
  db_manager.init();

  NetworkManager requestsListener(80, "127.0.0.1");
  requestsListener.setupSocket();
  requestsListener.startListening();

  return 0;
}
