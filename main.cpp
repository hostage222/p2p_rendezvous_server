#include <cstdlib>
#include <iostream>

#include "server.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: test_server <port>" << std::endl;
        return -1;
    }

    server::ptr p = server::create({static_cast<uint16_t>(atoi(argv[1]))});
    p->start();

    return 0;
}
