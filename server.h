#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <memory>

#include "connection.h"

class server : public server_interface,
        public std::enable_shared_from_this<server>
{
    server(uint16_t port);

public:
    using ptr = std::shared_ptr<server>;
    static ptr create(uint16_t port);
};

#endif // SERVER_H
