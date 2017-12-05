#include <iostream>
#include <signal.h>
#include <vector>
#include <queue>
#include <mutex>
#include <chrono>
#include <thread>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <getopt.h>

#include "handler.h"
#include "proxy.h"
#include "simple_resp.h"
#include "LogCabin/Client.h"
#include "ThreadPool.h"
#include "SessionManager.h"

#include "glog/logging.h"

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;
using LogCabin::Client::Result;
using LogCabin::Client::Status;

using simple_resp::encode_result;

static simple_resp::decoder dec;
static simple_resp::encoder enc;

std::unique_ptr<ThreadPool> pThreadPool;
std::unique_ptr<ThreadPool> pSenderThreadPool;
std::unique_ptr<logcabin_redis_proxy::handler> phandler;

static void
write_to_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    // Not yet implemented
}


static std::string
to_uppercase(std::string s)
{
    for (auto &i : s) {
        if (i >= 'a' && i <= 'z') {
            i = i - ('a' - 'A');
        }
    }
    return s;
}

void
sender_worker(std::string& resp, int command_id, int session_id)
{
    DLOG(INFO) << " command id:" << command_id << ", session id " << session_id << ", will reply";
    auto ctx = SessionManager::getInstance()->get_ctx(session_id);
    if(NULL != ctx && ctx->is_eof_reach == false)
    {
        ctx->insert_new_response(command_id, resp);
    }
    DLOG(INFO) << " command id:" << command_id << ", session id " << session_id << ",reply done";
}

void send_reply(int command_id, int session_id, std::string& response)
{
    pSenderThreadPool->enqueue(sender_worker, response, command_id, session_id);
}

void
processer_worker(int command_id, std::vector<std::string>& request, int session_id)
{
    encode_result encode_result;
    DLOG(INFO) << "command:" << request[0] <<", command id:" << command_id << ",session id is:" << session_id;
    std::string command = to_uppercase(request[0]);
    if (command == "RPUSH") 
    {
        encode_result = phandler->handle_rpush_request(request);
    }else if (command == "SADD") {
        encode_result = phandler->handle_sadd_request(request);
    }else if (command == "SREM") {
        encode_result = phandler->handle_srem_request(request);
    }else if (command == "MGET") {
        encode_result = phandler->handle_mget_request(request);
    }else if (command == "SMEMBERS") {
        encode_result = phandler->handle_smembers_request(request);
    }else if (command == "SCARD") {
        encode_result = phandler->handle_scard_request(request);
    }else if (command == "LPUSH") {
        encode_result = phandler->handle_lpush_request(request);
    } 
    else if (command == "LRANGE") {
        encode_result = phandler->handle_lrange_request(request);
    }
    else if (command == "LTRIM") 
    {
        encode_result = phandler->handle_ltrim_request(request);
    }
    else if (command == "EXPIRE") 
    {
        encode_result = phandler->handle_expire_request(request);
    }
    else if (command == "SET")
    {
        encode_result = phandler->handle_set_request(request);
    } 
    else if (command == "GET")
    {
        encode_result = phandler->handle_get_request(request);
    }
    else 
    {
        encode_result = enc.encode(simple_resp::ERRORS, {"ERR unknown command"});
    }
    send_reply(command_id, session_id, encode_result.response);
    DLOG(INFO) << "send reply to queue done for " << command_id << "," << session_id;
}

static void
read_from_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size] = {0};
    ssize_t size = read(fd, recv_buffer, buffer_size - 1);

    auto ctx = static_cast<simple_resp::decode_context*>(clientdata);
    if (size < 0) {
        LOG(ERROR) << "error happend: " << strerror(errno);
        aeDeleteFileEvent(loop, fd, mask);
        SessionManager::getInstance()->erase_session(ctx->get_session_id());
    } else if (size == 0) {
        DLOG(INFO) << "the connection is disconnected by client:" << fd <<", session id:" << ctx->get_session_id();
        aeDeleteFileEvent(loop, fd, mask);  // means client disconnected
        SessionManager::getInstance()->erase_session(ctx->get_session_id());
    } else {
        std::string command(recv_buffer, size);
        DLOG(INFO) << "command reach!";
        ctx->append_new_buffer(command);
        dec.decode(*ctx);
    }
}


static void
accept_tcp_handler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_port, client_fd;
    char client_ip[128];

    // create client socket
    client_fd = anetTcpAccept(nullptr, fd, client_ip, 128, &client_port);

    // set client socket non-block
    anetNonBlock(nullptr, client_fd);

    // register on message callback
    int new_session_id = SessionManager::getInstance()->insert_new_staus(client_fd);
     simple_resp::decode_context * ctx = new simple_resp::decode_context(new_session_id, [new_session_id](int command_id, std::vector<std::string>&response){
             pThreadPool->enqueue(processer_worker, command_id, response, new_session_id);
            });
    if(aeCreateFileEvent(loop, client_fd, AE_READABLE, read_from_client, (void*)ctx) == AE_ERR){
        LOG(ERROR) << "can not create file event for :" << client_fd;
        return;
    }
    DLOG(INFO) << "open event done for fd:" << fd << ", the client_fd is:" << client_fd;
}

class option_parser {
public:
    option_parser(int &argc, char** &argv)
        : argc(argc)
        , argv(argv)
        , set_size(1024)
        , service_port(6380)
        , cluster("127.0.0.1:5254")
        , bind_addr("0.0.0.0")
        , thread_num(10)
    {

        if (argc < 2) {
            usage();
            exit(1);
        }

        while (true) {
            static struct option longOptions[] = {
                {"cluster", required_argument, NULL, 'c'},
                {"help", no_argument, NULL, 'h'},
                {"size", required_argument, NULL, 's'},
                {"port", required_argument, NULL, 'p'},
                {"address", required_argument, NULL, 'a'},
                {"thread_num", required_argument, NULL, 't'},
                {0, 0, 0, 0}
            };
            int c = getopt_long(argc, argv, "c:hs:p:a:t:", longOptions, NULL);

            // Detect the end of the options
            if (c == -1)
                break;

            switch (c) {
                case 'c':
                    cluster = optarg;
                    break;
                case 's':
                    set_size = atoi(optarg);
                    break;
                case 'p':
                    service_port = atoi(optarg);
                    break;
                case 'a':
                    bind_addr = optarg;
                    break;
                case 't':
                    thread_num = atoi(optarg);
                    break;
                case 'h':
                    usage(),
                    exit(0);
                case '?':
                default:
                    // getopt_long already printed an error message.
                    usage();
                    exit(1);
            }
        }
    }

    void usage() {
        std::cout
        << "Usage: " << argv[0] << " [options]"
        << std::endl
        << std::endl

        << "Options:"
        << std::endl

        << "  -c <addresses>, --cluster=<addresses>  "
        << "Network addresses of the LogCabin, comma-separated (default: localhost:5254)"
        << std::endl

        << "  -h, --help "
        << "Print this usage information"
        << std::endl

        << "  -s <set_size> --size=<set_size> "
        << "Size of event loop [default: 1024]"
        << std::endl

        << "  -p <service_port> --port=<service_port> "
        << "Service port of proxy"
        << std::endl

        << "  -a <bind_addr> --address=<bind_addr> "
        << "Binding address of proxy"
        << std::endl

        << "  -t <thread_num> --thread_num=<num> "
        << "Num of working thread"
        << std::endl

        << std::endl;
    }

    int &argc;
    char** &argv;
    std::string cluster;
    std::string bind_addr;
    int service_port;
    int set_size;
    int thread_num;
};

int main(int argc, char** argv)
{
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::INFO);
    google::SetLogDestination(google::INFO, "./INFO_");
    google::SetLogDestination(google::ERROR, "./ERROR_");
    google::SetLogDestination(google::WARNING, "./WARNING_");

    try {
        signal(SIGPIPE, SIG_IGN);
        option_parser options(argc, argv);

        logcabin_redis_proxy::proxy proxy = {options.service_port, options.set_size, options.bind_addr,
                                             write_to_client, read_from_client, accept_tcp_handler};
        phandler.reset(new logcabin_redis_proxy::handler(options.cluster));
        pThreadPool.reset(new ThreadPool(static_cast<size_t>(options.thread_num)));
        pSenderThreadPool.reset(new ThreadPool(static_cast<size_t>(options.thread_num)));

        proxy.run();
    } catch (const LogCabin::Client::Exception &e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what();
        exit(1);
    }
    google::ShutdownGoogleLogging();
}
