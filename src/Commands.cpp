#include "Commands.h"

void registerCommands(pulse::utils::Console &console,
                      const pulse::net::NetworkManager<pulse::net::HttpMessageAssembler> &network_manager)
{
    console.registerCommand("connections",
                            [&](const std::vector<std::string> &args) { network_manager.showClients(std::cout); });
}
