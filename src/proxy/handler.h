#ifndef LOGCABIN_REDIS_PROXY_HANDLER_H
#define LOGCABIN_REDIS_PROXY_HANDLER_H

#include <string>
#include <vector>

#include "LogCabin/Client.h"
#include "simple_resp.h"

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;
using LogCabin::Client::Result;
using LogCabin::Client::Status;

using simple_resp::encode_result;

namespace logcabin_redis_proxy {
class handler {
public:
    explicit handler(Cluster& cluster) : pTree(std::make_shared<Tree>(cluster.getTree()))
    { }
    encode_result handle_rpush_request(const std::vector<std::string>& redis_args);
    encode_result handle_lpush_request(const std::vector<std::string>& redis_args);
    encode_result handle_lrange_request(const std::vector<std::string>& redis_args);
    encode_result handle_ltrim_request(const std::vector<std::string>& redis_args);
    encode_result handle_expire_request(const std::vector<std::string>& redis_args);
    encode_result handle_set_request(const std::vector<std::string>& redis_args);
    encode_result handle_get_request(const std::vector<std::string>& redis_args);
    ~handler() = default;

private:
    std::shared_ptr<Tree> pTree;
    simple_resp::decoder dec;
    simple_resp::encoder enc;

    // helper functions
    std::vector<std::string> split_list_elements(const std::string& original);

public:
    // handler is non-copyable.
    handler(const handler&) = delete;
    handler& operator=(const handler&) = delete;
};
} // namespace logcabin_redis_proxy

#endif
