#include "simple_resp.h"

namespace simple_resp {

    decode_context::decode_context(int session_id, request_handler req_handler){
        state = INIT;
        this->handler = req_handler;
        this->session_id = session_id;
        last_ack_command = 0;
        last_command_id = 0;
        buffered_input = "";
        reading_list_size = 0;
        req_list.clear();
    }
    void decode_context::pop_buffer(int start)
    {
        std::lock_guard<std::mutex> guard(buffer_mutex);
        buffered_input = buffered_input.substr(start, buffered_input.size() - start);
    }

    void decode_context::append_new_buffer(const std::string& buffer)
    {
        std::lock_guard<std::mutex> guard(buffer_mutex);
        buffered_input.append(buffer);
    }

    void decode_context::push_element(const std::string& element)
    {
        reading_str_size = 0;
        req_list.push_back(element);
        if(req_list.size() == reading_list_size)
        {
            handler(last_command_id, req_list);
            last_command_id ++;
            req_list.clear();
            reading_list_size = 0;
            state = INIT;
        }
        else
        {
            state = PARSE_ELEMENTS;
        }
    }

    void decoder::decode(decode_context& ctx)
    {
        if (ctx.buffered_input.length() <= 0) {
            return;
        }

        //decoder is used for coming request, so we just handle arrays
        parse(ctx);
    }

    void decoder::parse(decode_context& ctx)
    {
        int token_start = 0;
        for(auto i = 0; 
                i < ctx.buffered_input.length(); i++){

            if(ctx.buffered_input.at(i) == '\r' && 
                i + 1 != ctx.buffered_input.length() &&
                ctx.buffered_input.at(i + 1) == '\n')
            {

                //maybe you reach the end of a token

                auto token = ctx.buffered_input.substr(token_start, i - token_start);
                //most of the time, token should be all buffered, but there are some exception
                bool is_token_done = true;
                switch(ctx.state)
                {
                    case INIT:
                        {

                            //read the length of request
                            ctx.reading_list_size = 
                                static_cast<vector_len_type>(
                                        std::stoi(token.substr(1, token.length() - 1)));

                            ctx.state = PARSE_ELEMENTS;
                        }
                        break;
                    case PARSE_ELEMENTS:
                        {
                            switch(token[0])
                            {
                                //FIXME: copied from old source, I don't think this is right code
                                case INTEGERS:
                                    ctx.req_list.emplace_back(token);
                                    break;
                                case BULK_STRINGS:
                                    ctx.reading_str_size = static_cast<string_len_type>(std::stoi(
                                                token.substr(1, token.length() - 1)
                                                ));
                                    if(0 == ctx.reading_str_size)
                                    {
                                        //zero len str, just push back an empty string
                                        ctx.push_element("");
                                    }
                                    else
                                    {
                                        ctx.state = PARSE_BLUK_STRINGS;
                                        if(ctx.buffered_input.length() < i + ctx.reading_str_size){
                                            //the input is not fully buffered,
                                            //so don't waste your time, just return;
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    //unknow state!!
                                    break;
                            }
                        }
                        break;
                    case PARSE_BLUK_STRINGS:
                        if(token.length() < ctx.reading_str_size)
                        {
                            //this '\n\r' might be one the context of string
                            is_token_done = false;
                        }
                        else if(token.length() > ctx.reading_str_size)
                        {
                            //something might go wrong
                            //should set up a handler to handle error
                        }
                        else
                        {
                            ctx.push_element(token);
                        }
                }

                if(is_token_done){
                    //skip '\n'
                    i++;

                    //move to the next char after '\n'
                    token_start = i + 1;
                }
            }
        }
        ctx.pop_buffer(token_start);
    }

    encode_result encoder::encode(const RESP_TYPE &type, const std::vector<std::string> &args)
    {
        encode_result result;

        result.status = OK;

        switch (type) {
            case SIMPLE_STRINGS:
                result.response = "+" + args[0] + "\r\n";  // only takes the first element and ignore rest
                break;
            case ERRORS:
                result.response = "-" + args[0] + "\r\n";  // only takes the first element and ignore rest
                break;
            case INTEGERS:
                result.response = "$" + std::to_string(args[0].length()) + "\r\n" + args[0] + "\r\n";
                break;
            case BULK_STRINGS:
                result.response = "$" + std::to_string(args[0].length()) + "\r\n" + args[0] + "\r\n";
                break;
            case ARRAYS:
                result.response = "*" + std::to_string(args.size()) + "\r\n";
                for (auto it = args.begin(); it != args.end(); ++it) {
                    result.response += "$" + std::to_string(it->length()) + "\r\n" + *it + "\r\n";
                }
                break;
        }
        return result;
    }
} // namespace simple_resp
