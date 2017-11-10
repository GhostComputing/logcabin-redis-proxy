#include "proxy.h"

namespace logcabin_redis_proxy {

bool
proxy::run()
{
    int ipfd = anetTcpServer(nullptr, service_port_, const_cast<char *>(bind_addr_.c_str()), 1024);
    if(ipfd == ANET_ERR){
        return false;
    }

    int ret = aeCreateFileEvent(loop, ipfd, AE_READABLE, accept_handler_, nullptr);
    assert(ret != AE_ERR);

    aeMain(loop);
    return true;
}

} // namespace logcabin_redis_proxy
