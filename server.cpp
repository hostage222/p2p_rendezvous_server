#include "server.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <boost/asio.hpp>

#include "connection.h"
#include "log.h"

using namespace std;
using namespace boost::asio;
using boost::asio::ip::tcp;
using boost_error = boost::system::error_code;

server::server(uint16_t port) :
    acceptor{service, tcp::endpoint{tcp::v4(), port}}
{
}

server::ptr server::create(uint16_t port)
{
    server *p = new server(port);
    return ptr{p};
}

void server::start()
{
    wait_connection();
    service.run();
}

void server::remove_client(connection::ptr con)
{
    (void)con;
}

void server::handle_accept(connection::ptr new_connection,
                           boost::system::error_code error)
{
    if (error)
    {
        return;
    }
    log() << "connected " << new_connection->socket().remote_endpoint();

    new_connection->start();
    wait_connection();
}

void server::wait_connection()
{
    auto client = connection::create(shared_from_this(), service);
    acceptor.async_accept(client->socket(),
                          [this, client](boost_error error)
                          {handle_accept(client, error);});
}
