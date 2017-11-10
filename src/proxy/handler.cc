#include "handler.h"

namespace logcabin_redis_proxy {
    
encode_result
handler::handle_lpush_request(const std::vector<std::string> &redis_args)
{
    try {
        if (redis_args.size() < 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'lpush' command"});
        }
        std::string key = redis_args[1];
        for (auto it = redis_args.begin() + 2; it != redis_args.end(); ++it) {
            pTree->lpushEx(key, *it);   // FIXME: just ignore status because server is not yet supported
        }
        return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}


encode_result
handler::handle_rpush_request(const std::vector<std::string> &redis_args)
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
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_lrange_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() != 4) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'lrange' command"});
        }
        //FIXME: need to detect return value but server is not yet support
        return enc.encode(simple_resp::ARRAYS, split_list_elements(pTree->lrange(redis_args[1], redis_args[2] + " " + redis_args[3])));
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_ltrim_request(const std::vector<std::string>& redis_args)
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

encode_result
handler::handle_expire_request(const std::vector<std::string>& redis_args)
{
    try {
        Result result;
        if (redis_args.size() != 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'expire' command"});
        }

        result = pTree->expire(redis_args[1], redis_args[2]);

        if (result.status == Status::OK) {
            return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
        } else if (result.status == Status::INVALID_ARGUMENT) {
            return enc.encode(simple_resp::ERRORS, {"ERR invalid arguments for 'expire' command"});
        } else {
            return enc.encode(simple_resp::ERRORS, {"ERR unknown error for 'expire' command"});
        }
    } catch (const LogCabin::Client::Exception& e) {
        std::cerr << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  << std::endl;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

//TODO: need to refractor such ugly code
std::vector<std::string>
handler::split_list_elements(const std::string& original) {
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

} // namespace logcabin_redis_proxy
