#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>

class server_interface
{
public:

};

class connection
{
public:
    using server_interface_ptr = std::weak_ptr<server_interface>;
    connection(server_interface_ptr server);

private:
    server_interface_ptr server;
};

#endif // CONNECTION_H
