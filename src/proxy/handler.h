#ifndef LOGCABIN_REDIS_PROXY_HANDLER_H
#define LOGCABIN_REDIS_PROXY_HANDLER_H

#include <string>
#include <vector>
#include <mutex>

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
    explicit handler(const std::string& cluster_opts):ptree_counter(0)
    { 
        pTreeList.resize(10);
        for(int i = 0 ; i < 10 ; i ++)
        {
            Cluster cluster(cluster_opts);
            pTreeList[i] = std::make_shared<Tree>(cluster.getTree());
        }
    }
    encode_result handle_rpush_request(const std::vector<std::string>& redis_args);
    encode_result handle_lpush_request(const std::vector<std::string>& redis_args);
    encode_result handle_lrange_request(const std::vector<std::string>& redis_args);
    encode_result handle_ltrim_request(const std::vector<std::string>& redis_args);
    encode_result handle_expire_request(const std::vector<std::string>& redis_args);
    encode_result handle_set_request(const std::vector<std::string>& redis_args);
    encode_result handle_get_request(const std::vector<std::string>& redis_args);
    ~handler() = default;

private:

    std::shared_ptr<Tree> getTree();

    simple_resp::decoder dec;
    simple_resp::encoder enc;

    int ptree_counter;
    std::mutex ptree_counter_lock;

    std::vector<std::shared_ptr<Tree>> pTreeList;
    // helper functions
    std::vector<std::string> split_list_elements(const std::string& original);

public:
    // handler is non-copyable.
    handler(const handler&) = delete;
    handler& operator=(const handler&) = delete;
};
} // namespace logcabin_redis_proxy

#endif
