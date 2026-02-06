#pragma once
#include "networking/NetworkManager.h"
#include "networking/http/HttpAssembler.h"
#include "utils/Console.h"
#include <string>
#include <vector>

void registerCommands(pulse::utils::Console &console,
                      const pulse::net::NetworkManager<pulse::net::HttpAssembler> &network_manager);
