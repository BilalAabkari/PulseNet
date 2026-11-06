#include "TLSAssembler.h"

namespace pulse::net
{

TLSAssembler::TLSAssembler()
{
}

TLSAssembler::AssemblingResult TLSAssembler::feed(uint64_t id, char *buffer, int &buffer_len, int max_buffer_len,
                                                  int last_tcp_packet_len)
{
}

} // namespace pulse::net
