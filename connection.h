#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <array>
#include <map>
#include <boost/asio.hpp>

#include "p2p_common.h"

class connection;

class server_interface
{
public:
    virtual void remove_client(std::shared_ptr<connection> con) = 0;
    virtual p2p::version_type get_version() const = 0;
};

class connection : public std::enable_shared_from_this<connection>
{
public:
    using server_interface_ptr = std::weak_ptr<server_interface>;
    ~connection();

private:
    connection(server_interface_ptr server, boost::asio::io_service &service);

public:
    using ptr = std::shared_ptr<connection>;
    static ptr create(server_interface_ptr server,
                      boost::asio::io_service &service);

    void start();

    boost::asio::ip::tcp::socket &socket() { return sock; }

private:
    server_interface_ptr server;
    boost::asio::ip::tcp::socket sock;

    static constexpr size_t MAX_BUF_SIZE = 10000;
    std::array<uint8_t, MAX_BUF_SIZE> buf;
    size_t buf_size;

    std::string phone;

    void start_read();
    size_t read_complete(boost::system::error_code error, size_t bytes);
    void read(boost::system::error_code error, size_t bytes);
    void start_write();

    void process_request();
    using handler = void(connection::*)(p2p::buf_sequence params);
    static const std::map<std::string, handler> handlers;

    void get_version_handler(p2p::buf_sequence params);
    void register_handler(p2p::buf_sequence params);
    void unregister_handler(p2p::buf_sequence params);
    void autorize_handler(p2p::buf_sequence params);

    bool test_emptiness(p2p::buf_sequence params);
};

#endif // CONNECTION_H
