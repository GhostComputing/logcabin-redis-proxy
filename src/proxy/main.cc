#include <iostream>
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

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;
using LogCabin::Client::Result;
using LogCabin::Client::Status;

struct request {
    int fd;
    std::string cmd;
};

static std::queue<request> request_queue;
static simple_resp::decoder dec;
static simple_resp::encoder enc;
std::mutex request_queue_mutex;

static void write_to_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    // Not yet implemented
}

static void reply(int fd, char *send_buffer, std::string &content)
{
    memcpy(send_buffer, content.c_str(), content.length());
    write(fd, send_buffer, content.length());
}

static std::string to_uppercase(std::string s)
{
    for (auto &i : s) {
        if (i >= 'a' && i <= 'z') {
            i = i - ('a' - 'A');
        }
    }
    return s;
}

static void process_req(const std::vector<request> &req, logcabin_redis_proxy::handler& handler)
{
    simple_resp::decode_result decode_result;
    simple_resp::encode_result encode_result;

    char send_buffer[1024] = {'0'};

    for (const request & r : req) {
        decode_result = dec.decode(r.cmd);
        if (decode_result.status == simple_resp::OK) {
            std::string command = std::move(to_uppercase(decode_result.response[0]));
            if (command == "RPUSH") {
                encode_result = handler.handle_rpush_request(decode_result.response);
            } else if (command == "LRANGE") {
                encode_result = handler.handle_lrange_request(decode_result.response);
            } else if (command == "LTRIM") {
                encode_result = handler.handle_ltrim_request(decode_result.response);
            } else {
                encode_result = enc.encode(simple_resp::ERRORS, {"ERR unknown command"});
            }
        } else {
            encode_result = enc.encode(simple_resp::ERRORS, {"ERR unknown internal error"});
        }
        reply(r.fd, send_buffer, encode_result.response);
    }
}

void worker(int thread_num, logcabin_redis_proxy::handler& handler)
{
    int i;
    std::vector<request> handle_requests;
    while(1) {
        request_queue_mutex.lock();
        i = 0;
        int handle_req_num = request_queue.size() / thread_num > 0 ? int(request_queue.size() / thread_num):int(request_queue.size());
        while (!request_queue.empty()) {
            if (++i > handle_req_num) {
                break;
            }
            request r = request_queue.front();
            handle_requests.push_back(r);
            request_queue.pop();
        }
        request_queue_mutex.unlock();
        process_req(handle_requests, handler);
        handle_requests.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

}

static void read_from_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size] = {0};
    ssize_t size = read(fd, recv_buffer, buffer_size);

    if (size < 0) {
        std::cerr << "error happend: " << strerror(errno) << std::endl;
        aeDeleteFileEvent(loop, fd, mask);
    } else if (size == 0) {
        aeDeleteFileEvent(loop, fd, mask);  // means client disconnected
    } else {
        std::string command(recv_buffer);
        request req = {fd, command};
        request_queue.emplace(req);
    }
}

static void accept_tcp_handler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_port, client_fd;
    char client_ip[128];

    // create client socket
    client_fd = anetTcpAccept(nullptr, fd, client_ip, 128, &client_port);

    // set client socket non-block
    anetNonBlock(nullptr, client_fd);

    // register on message callback
    assert(aeCreateFileEvent(loop, client_fd, AE_READABLE, read_from_client, nullptr) != AE_ERR);
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
    try {
        option_parser options(argc, argv);
        Cluster cluster(options.cluster);

        logcabin_redis_proxy::handler handler(cluster);
        logcabin_redis_proxy::proxy proxy = {options.service_port, options.set_size, options.bind_addr,
                                             write_to_client, read_from_client, accept_tcp_handler};
        std::vector<std::thread> threads;

        // TODO: maybe need to join
        for (int i = 0; i != options.thread_num; ++i) {
            threads.emplace_back(std::thread(worker, options.thread_num, std::ref(handler)));
        }

        proxy.run();
    } catch (const LogCabin::Client::Exception &e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        exit(1);
    }
}
