#include "connection.h"

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <array>
#include <boost/asio.hpp>

#include "p2p_common.h"
#include "database.h"

using namespace std;
using namespace p2p;
using namespace boost::asio;
using boost::asio::ip::tcp;
using boost_error = boost::system::error_code;

const std::map<std::string, connection::handler> connection::handlers = {
    {p2p::GET_VERSION, connection::get_version_handler},
    {p2p::REGISTER, connection::register_handler},
    {p2p::UNREGISTER, connection::unregister_handler},
    {p2p::AUTORIZE, connection::autorize_handler}
};

connection::connection(server_interface_ptr server,
                       boost::asio::io_service &service) :
    server{server}, sock{service}
{
}

connection::~connection()
{
    database::instance().unautorize(phone);
}

connection::ptr connection::create(connection::server_interface_ptr server,
                                   boost::asio::io_service &service)
{
    auto p = new connection{server, service};
    return ptr{p};
}

void connection::start()
{
    start_read();
}

void connection::start_read()
{
    async_read(sock, buffer(buf),
               [self = shared_from_this()](boost_error error, size_t bytes)
               { return self->read_complete(error, bytes); },
               [self = shared_from_this()](boost_error error, size_t bytes)
               { self->read(error, bytes); });
}

size_t connection::read_complete(boost_error error, size_t bytes)
{
    if (error)
    {
        server.lock()->remove_client(shared_from_this());
        return 0;
    }

    return p2p::read_complete(buf, bytes);
}

void connection::read(boost_error error, size_t bytes)
{
    if (error)
    {
        server.lock()->remove_client(shared_from_this());
        return;
    }

    if (p2p::is_valid_message(buf, bytes))
    {
        buf_size = bytes;
        process_request();
    }
    else
    {
        p2p::write_command(buf, buf_size, p2p::INVALID_FORMAT);
    }
    p2p::finalize(buf, buf_size);

    start_write();
}

void connection::start_write()
{
    async_write(sock, buffer(buf, buf_size),
                [self = shared_from_this()](boost_error ec, size_t)
                { if (ec) self->server.lock()->remove_client(self);
                  else self->start_read(); });
}

void connection::process_request()
{
    buf_sequence buf_seq = get_buf_sequence(buf, buf_size);

    try
    {
        string cmd = p2p::read_string(buf_seq);
        auto it = handlers.find(cmd);
        if (it != handlers.end())
        {
            auto fun = it->second;
            (this->*fun)(buf_seq);
        }
        else
        {
            p2p::write_command(buf, buf_size, p2p::INVALID_COMMAND);
        }
    }
    catch (invalid_token_exception&)
    {
        p2p::write_command(buf, buf_size, p2p::INVALID_FORMAT);
    }
}

void connection::get_version_handler(buf_sequence params)
{
    if (!test_emptiness(params))
    {
        return;
    }

    auto version = server.lock()->get_version();
    p2p::write_command(buf, buf_size, p2p::GET_VERSION);
    p2p::append_param(buf, buf_size, p2p::to_string(version));
}

void connection::register_handler(buf_sequence params)
{
    string phone = p2p::read_string(params);
    string password = p2p::read_string(params);
    string code = p2p::read_string(params);
    if (!test_emptiness(params))
    {
        return;
    }

    string res = database::instance().register_(phone, password, code);
    p2p::write_command(buf, buf_size, p2p::REGISTER);
    p2p::append_param(buf, buf_size, res);
}

void connection::unregister_handler(buf_sequence params)
{
    string phone = p2p::read_string(params);
    string password = p2p::read_string(params);
    if (!test_emptiness(params))
    {
        return;
    }

    string res = database::instance().unregister(phone, password);
    p2p::write_command(buf, buf_size, p2p::UNREGISTER);
    p2p::append_param(buf, buf_size, res);
}

void connection::autorize_handler(buf_sequence params)
{
    phone = p2p::read_string(params);
    string password = p2p::read_string(params);
    if (!test_emptiness(params))
    {
        return;
    }

    string res = database::instance().autorize(phone, password);
    p2p::write_command(buf, buf_size, p2p::AUTORIZE);
    p2p::append_param(buf, buf_size, res);
}

bool connection::test_emptiness(buf_sequence params)
{
    if (!p2p::is_empty(params))
    {
        p2p::write_command(buf, buf_size, p2p::INVALID_DATA);
        return false;
    }
    return true;
}
