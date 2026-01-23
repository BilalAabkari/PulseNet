#include "HttpRequest.h"

namespace pulse::net
{

NetEvent::Protocol HttpRequest::protocol()
{
    return NetEvent::Protocol::HTTP;
}

} // namespace pulse::net
