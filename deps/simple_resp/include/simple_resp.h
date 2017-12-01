#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <mutex>

#ifndef SIMPLE_RESP_SIMPLE_RESP_H
#define SIMPLE_RESP_SIMPLE_RESP_H

namespace simple_resp {

using vector_len_type = std::vector<std::string>::size_type;
using string_len_type = std::string::size_type;

enum RESP_TYPE {
    SIMPLE_STRINGS = '+',
    ERRORS = '-',
    INTEGERS = ':',
    BULK_STRINGS = '$',
    BULK_NIL,
    ARRAYS = '*'
};

enum STATUS {
    OK = 0,
    EMPTY_INPUT = 1,
    INVAILID_RESP_TYPE = 2,
    INVAILID_RESP_FORMAT = 3,
    UNKNOWN_INTERNAL_ERROR = 3,
};

enum PARSE_STATE {
    INIT = 0,
    PARSE_ELEMENTS = 1,
    PARSE_BLUK_STRINGS = 2
};

struct encode_result {
    STATUS status;
    std::string response;
};

typedef struct _redis_type_value_pair{
    RESP_TYPE type;
    std::string value;
}redis_type_value_pair;
typedef std::vector<redis_type_value_pair> redis_type_value_pair_list;

using request_handler = std::function<void(int command_id, std::vector<std::string>&)>;
class decode_context{
    public:
        decode_context(int session_id, request_handler); 
        ~decode_context(){};

        void append_new_buffer(const std::string& buffer);
        void pop_buffer(int start);
        void push_element(const std::string& element);

        int get_session_id() { return session_id;}

    private:
        std::mutex buffer_mutex;
        int session_id;
        int last_ack_command;
        int last_command_id;
        request_handler handler;
        PARSE_STATE state;

        vector_len_type reading_list_size;
        string_len_type reading_str_size;
        std::string buffered_input;
        std::vector<std::string> req_list;

    friend class decoder;
};

class decoder {
public:
    decoder() = default;
    void decode(decode_context& ctx);

private:
    void parse(decode_context& ctx);

public:
    // decoder is non-copyable.
    decoder(const decoder &) = delete;
    decoder operator= (const decoder &) = delete;
};

class encoder {
public:
    encoder() = default;
    encode_result encode(const RESP_TYPE &type, const std::vector<std::string> &args);
    encode_result encode(const redis_type_value_pair_list& list);

    // Encoder is non-copyable.
    encoder(const encoder &) = delete;
    encoder operator= (const encoder &) = delete;
};

} // namespace simple_resp

#endif //SIMPLE_RESP_SIMPLE_RESP_H
