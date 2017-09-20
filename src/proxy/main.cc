#include <iostream>
#include <vector>
#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "proxy.h"
#include "LogCabin/Client.h"
#include "simple_resp.h"

using LogCabin::Client::Cluster;
using LogCabin::Client::Tree;

static std::shared_ptr<Tree> pTree = nullptr;

static void write_to_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    // Not yet implemented
}

static void reply(int fd, char *send_buffer, std::string &content)
{
    std::cout << "reply:" << content << std::endl;
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

static void handle_rpush_request(const std::vector<std::string>& redis_args, simple_resp::Encoder &enc)
{
    try {
        std::string key = redis_args[1];
        for (auto it = redis_args.begin() + 2; it != redis_args.end(); ++it) {
            pTree->rpushEx(key, *it);
        }
        enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        enc.encode(simple_resp::ERRORS, {"ERR Internal error happened\""});
    }
}

static std::vector<std::string> split_list_elements(const std::string &original) {
    std::string s(original);
    std::vector<std::string> elements;

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

static void handle_lrange_request(const std::vector<std::string>& redis_args, simple_resp::Encoder &enc)
{
    try {
        enc.encode(simple_resp::ARRAYS, std::move(split_list_elements(pTree->readEx(redis_args[1]))));
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        enc.encode(simple_resp::ERRORS, {"ERR Internal error happened\""});
    }
}

static void handle_ltrim_request(const std::vector<std::string>& redis_args, simple_resp::Encoder &enc)
{
    try {
        pTree->ltrim(redis_args[1], redis_args[2]);
        enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        enc.encode(simple_resp::ERRORS, {"ERR Internal error happened\""});
    }
}

// TODO: Handle the connection event in C++11 style
// TODO: Change async way to handle this
static void read_from_client(aeEventLoop *loop, int fd, void *clientdata, int mask)
{
    const int buffer_size = 1024;
    char recv_buffer[buffer_size] = {0};
    char send_buffer[buffer_size] = {0};
    simple_resp::Decoder dec;
    simple_resp::Encoder enc;

    ssize_t size = read(fd, recv_buffer, buffer_size);
    if (size <= 0) {
        aeDeleteFileEvent(loop, fd, mask);
    } else {
        if (dec.decode(recv_buffer) == simple_resp::OK) {
            std::string command = std::move(to_uppercase(dec.decoded_redis_command[0]));
            if (command == "RPUSH") {
                handle_rpush_request(dec.decoded_redis_command, enc);
            } else if (command == "LRANGE") {
                handle_lrange_request(dec.decoded_redis_command, enc);
            } else if (command == "LTRIM") {
                handle_ltrim_request(dec.decoded_redis_command, enc);
            } else {
                enc.encode(simple_resp::ERRORS, {"ERR unknown command"});
            }
        } else {
            enc.encode(simple_resp::ERRORS, {"ERR unknown internal error"});
            aeDeleteFileEvent(loop, fd, mask);
        }
        reply(fd, send_buffer, enc.encoded_redis_command);
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

int main()
{
    Cluster cluster("127.0.0.1:5254");
    Tree tree = cluster.getTree();
    pTree = std::make_shared<Tree>(tree);
    logcabin_redis_proxy::proxy proxy = {6380, 1024, "0.0.0.0", write_to_client, read_from_client, accept_tcp_handler};
    proxy.run();
}
