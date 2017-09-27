#include <iostream>
#include <vector>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <getopt.h>

#include "proxy.h"
#include "LogCabin/Client.h"
#include "simple_resp.h"

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;
using LogCabin::Client::Result;
using LogCabin::Client::Status;

static std::shared_ptr<Tree> pTree = nullptr;

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

static std::string handle_rpush_request(const std::vector<std::string>& redis_args, simple_resp::encoder &enc)
{
    try {
        if (redis_args.size() < 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'rpush' command"});
        }
        std::string key = redis_args[1];
        for (auto it = redis_args.begin() + 2; it != redis_args.end(); ++it) {
            pTree->rpushEx(key, *it);   // FIXME: just ignore status because server is not yet supported
        }
        return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened\""});
    }
}

//TODO: need to refractor such ugly code
static std::vector<std::string> split_list_elements(const std::string &original) {
    std::vector<std::string> elements;
    if (original.length() < 0) {
        return elements;
    } else {
        std::string s(original);
        std::string delimiter1 = ",";
        std::string delimiter2 = ":";
        std::string token;

        size_t pos = 0;
        int counter = 0;

        while ((pos = s.find(delimiter1)) != std::string::npos) {
            token = s.substr(0, pos);
            elements.push_back(token);
            s.erase(0, pos + delimiter1.length());
        }

        for (auto it = elements.begin(); it != elements.end(); it++) {
            counter = 0;
            while ((pos = it->find(delimiter2)) != std::string::npos && counter <= 4) {
                counter++;
                token = it->substr(0, pos);
                it->erase(0, pos + delimiter2.length());
            }
        }

        return elements;
    }
}

static std::string handle_lrange_request(const std::vector<std::string>& redis_args, simple_resp::encoder &enc)
{
    try {
        if (redis_args.size() != 4) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'lrange' command"});
        }
        //FIXME: need to detect return value but server is not yet support
        return enc.encode(simple_resp::ARRAYS, std::move(split_list_elements(pTree->lrange(redis_args[1], redis_args[2] + " " + redis_args[3]))));
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened\""});
    }
}

static std::string handle_ltrim_request(const std::vector<std::string>& redis_args, simple_resp::encoder &enc)
{
    try {
        Result result;
        if (redis_args.size() != 4) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'ltrim' command"});
        }

        result = pTree->ltrim(redis_args[1], redis_args[2] + " " + redis_args[3]);

        if (result.status == Status::OK) {
            return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
        } else if (result.status == Status::INVALID_ARGUMENT) {
            return enc.encode(simple_resp::ERRORS, {"ERR invalid arguments for 'ltrim' command"});
        } else {
            return enc.encode(simple_resp::ERRORS, {"ERR unknown error for 'ltrim' command"});
        }
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

// TODO: Handle the connection event in C++11 style
// TODO: Change async way to handle this
static void read_from_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size] = {0};
    char send_buffer[buffer_size] = {0};
    simple_resp::decoder dec;
    simple_resp::encoder enc;
    std::string response;

    ssize_t size = read(fd, recv_buffer, buffer_size);
    if (size <= 0) {
        aeDeleteFileEvent(loop, fd, mask);
    } else {
        if (dec.decode(recv_buffer) == simple_resp::OK) {
            std::string command = std::move(to_uppercase(dec.decoded_redis_command[0]));
            if (command == "RPUSH") {
                response = handle_rpush_request(dec.decoded_redis_command, enc);
            } else if (command == "LRANGE") {
                response = handle_lrange_request(dec.decoded_redis_command, enc);
            } else if (command == "LTRIM") {
                response = handle_ltrim_request(dec.decoded_redis_command, enc);
            } else {
                response = enc.encode(simple_resp::ERRORS, {"ERR unknown command"});
            }
        } else {
            response = enc.encode(simple_resp::ERRORS, {"ERR unknown internal error"});
            aeDeleteFileEvent(loop, fd, mask);
        }
        reply(fd, send_buffer, response);
    }
}

static void accept_tcp_handler(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    int client_port, client_fd;
    char client_ip[128];

    // create client socket
    client_fd = anetTcpAccept(nullptr, fd, client_ip, 128, &client_port);
    std::cout << "Accepted " << client_ip << ":" << client_port << std::endl;

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
                {0, 0, 0, 0}
            };
            int c = getopt_long(argc, argv, "c:hs:p:a:", longOptions, NULL);

            // Detect the end of the options
            if (c == -1)
                break;

            switch (c) {
                case 'c':
                    cluster = optarg;
                    break;
                case 's':
                    set_size = static_cast<int>(atoi(optarg));
                    break;
                case 'p':
                    service_port = static_cast<int>(atoi(optarg));
                    break;
                case 'a':
                    bind_addr = optarg;
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

        << std::endl;
    }

    int &argc;
    char** &argv;
    std::string cluster;
    std::string bind_addr;
    int service_port;
    int set_size;
};

int main(int argc, char** argv)
{
    try {
        option_parser options(argc, argv);
        Cluster cluster(options.cluster);
        Tree tree = cluster.getTree();
        pTree = std::make_shared<Tree>(tree);
        logcabin_redis_proxy::proxy proxy = {options.service_port, options.set_size, options.bind_addr,
                                             write_to_client, read_from_client, accept_tcp_handler};
        proxy.run();
    } catch (const LogCabin::Client::Exception &e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        exit(1);
    }
}
