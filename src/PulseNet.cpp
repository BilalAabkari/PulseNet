#include "PulseNet.h"
#include "utils/ConfigParser.h"
#include "database/DatabaseConnection.h"

int main() {

  ConfigParser parser;
  parser.read();
  std::cout << parser;

  DatabaseKeys db_config = parser.getDatabaseConfig();
  DatabaseConnection db(db_config.database_host, db_config.database_port,
                        db_config.database_password);

  bool should_create_db = db.connect();
  if (db.isConnected()) {
    std::cout << "Connected to database successfuly\n";
  }

  return 0;
}
