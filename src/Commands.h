#pragma once
#include "networking/NetworkManager.h"
#include "utils/Console.h"
#include <string>
#include <vector>

void registerCommands(pulse::utils::Console &console, const pulse::net::NetworkManager &network_manager);
