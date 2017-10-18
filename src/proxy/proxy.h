#include <iostream>
#include <cassert>
#include <string>

#include "ae/ae.h"
#include "ae/anet.h"

#ifndef LOGCABIN_REDIS_PROXY_SERVER_H
#define LOGCABIN_REDIS_PROXY_SERVER_H

namespace logcabin_redis_proxy {

const int DEFAULT_SERVICE_PORT = 6380;
const int DEFAULT_SET_SIZE = 1024;

class proxy {
public:
    proxy() = default;
    proxy(const int &service_port, const int &set_size, const std::string &bind_addr,
           aeFileProc *writable_handler, aeFileProc *readable_handler, aeFileProc *accept_handler)
           : service_port_(service_port)
           , set_size_(set_size)
           , bind_addr_(bind_addr)
           , loop(aeCreateEventLoop(set_size))
           , writeable_handler_(writable_handler)
           , readable_handler_(readable_handler)
           , accept_handler_(accept_handler)
    { }

    bool run();
private:
    int service_port_ = DEFAULT_SERVICE_PORT;
    int set_size_ = DEFAULT_SET_SIZE;
    std::string bind_addr_ = "0.0.0.0";
    aeEventLoop *loop = nullptr;
    aeFileProc *writeable_handler_ = nullptr;
    aeFileProc *readable_handler_ = nullptr;
    aeFileProc *accept_handler_ = nullptr;

public:
    // proxy is non-copyable.
    proxy(const proxy&) = delete;
    proxy& operator=(const proxy&) = delete;
};

} // namespace logcabin_redis_proxy

#endif //LOGCABIN_REDIS_PROXY_SERVER_H
