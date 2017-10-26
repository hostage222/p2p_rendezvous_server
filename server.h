#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <boost/asio.hpp>

#include "p2p_common.h"
#include "connection.h"

class server : public server_interface,
        public std::enable_shared_from_this<server>
{
    static constexpr p2p::version_type version = {1, 2, 3};
    server(uint16_t port);

public:
    using ptr = std::shared_ptr<server>;
    static ptr create(uint16_t port);
    void start();

private:
    void remove_client(connection::ptr con) override;
    p2p::version_type get_version() const override { return version; }

    boost::asio::io_service service;
    boost::asio::ip::tcp::acceptor acceptor;

    void handle_accept(connection::ptr new_connection,
                       boost::system::error_code error);
    void wait_connection();
};

#endif // SERVER_H
