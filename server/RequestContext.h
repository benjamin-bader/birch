#ifndef BIRCH_SERVER_REQUESTCONTEXT_H
#define BIRCH_SERVER_REQUESTCONTEXT_H

#include <array>
#include <cstdint>

namespace birch { namespace server {

class RequestContext
{
public:
    RequestContext();



private:
    // A buffer which will hold all data read from the remote connection.
    // Per RFC, messages MUST NOT exceed 512 bytes in length.
    std::array<char, 512> m_buffer;
};

}} // namespace birch::server

#endif