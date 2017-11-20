#include <iostream>

#include "simple_resp.h"

int main()
{
    simple_resp::decoder dec;
    std::string input("*3\r\n$3\r\nSET\r\n$1\r\na\r\n$1\r\nb\r\n*3\r\n$3\r\nSET\r\n$1\r\na\r\n$1\r\nc\r\n");
    simple_resp::decode_context ctx(0, [](int command_id, std::vector<std::string>& result){

            std::cout << "command id "<< command_id << " decode OK" << std::endl;
            for (const auto &c : result) {
            std::cout << c << " ";
            }
            std::cout << std::endl;
            });
    ctx.append_new_buffer(input);

    dec.decode(ctx);
    return 0;
}
