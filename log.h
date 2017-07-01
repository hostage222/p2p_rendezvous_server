#ifndef LOG_H_
#define LOG_H_

#include <iostream>

#include <boost/asio.hpp>

#include "p2p_common.h"

class _log
{
public:
    ~_log()
    {
        std::cout << std::endl;
    }

    template <typename T>
    _log& operator<<(const T &v)
    {
        std::cout << v;
        return *this;
    }

    _log& operator<<(const boost::asio::ip::tcp::endpoint &endpoint)
    {
        std::cout << p2p::to_string(endpoint);
    }

private:
    log() {}
    friend _log log();
};

inline _log log() { return _log(); }

#endif // LOG_H_
