#include "handler.h"
#include "glog/logging.h"

namespace logcabin_redis_proxy {

    std::shared_ptr<Tree> handler::getTree(){
        std::lock_guard<std::mutex> guard(ptree_counter_lock);
        std::shared_ptr<Tree> ret = pTreeList[ptree_counter % 10];
        ptree_counter++;
        ptree_counter %= 10;
        return ret;
    }

encode_result
handler::handle_sadd_request(const std::vector<std::string> &redis_args)
{
    try {
        if (redis_args.size() < 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'sadd' command"});
        }
        std::string key = redis_args[1];

        auto it = redis_args.begin();
        it ++;
        it ++;
        auto args = std::vector<std::string>(it, redis_args.end());

        getTree()->saddEx(key, args);   // FIXME: just ignore status because server is not yet supported
        encode_result result = enc.encode(simple_resp::INTEGERS, {"1"});
        return result;
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result handler::handle_srem_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() != 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'srem' command"});
        }
        std::string key = redis_args[1];
        getTree()->sremEx(key, redis_args[2]);   // FIXME: just ignore status because server is not yet supported
        encode_result result = enc.encode(simple_resp::INTEGERS, {"1"});
        return result;
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}
    
encode_result
handler::handle_lpush_request(const std::vector<std::string> &redis_args)
{
    try {
        if (redis_args.size() < 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'lpush' command"});
        }
        std::string key = redis_args[1];
        for (auto it = redis_args.begin() + 2; it != redis_args.end(); ++it) {
            getTree()->lpushEx(key, *it);   // FIXME: just ignore status because server is not yet supported
        }
        encode_result result = enc.encode(simple_resp::INTEGERS, {"1"});
        return result;
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_set_request(const std::vector<std::string> &redis_args)
{
    try {
        if (redis_args.size() < 3) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'set' command"});
        }
        std::string key = redis_args[1];
        getTree()->writeEx(key, redis_args[2]); 
        return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
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
            getTree()->rpushEx(key, *it);   // FIXME: just ignore status because server is not yet supported
        }
        return enc.encode(simple_resp::INTEGERS, {"1"});
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_scard_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() != 2) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'read' command"});
        }
        //FIXME: need to detect return value but server is not yet support
        auto result = getTree()->scard(redis_args[1]);

        auto encodeResult = enc.encode(simple_resp::INTEGERS, {result});

        return encodeResult;
    } catch (const LogCabin::Client::LookupException& e) {
        return enc.encode(simple_resp::BULK_NIL, {""});
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}


encode_result
handler::handle_get_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() != 2) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'read' command"});
        }
        //FIXME: need to detect return value but server is not yet support
        auto result = getTree()->readEx(redis_args[1]);

        auto encodeResult = enc.encode(simple_resp::BULK_STRINGS, {result});

        return encodeResult;
    } catch (const LogCabin::Client::LookupException& e) {
        return enc.encode(simple_resp::BULK_NIL, {""});
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_mget_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() < 2) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'smembers' command"});
        }
        //FIXME: need to detect return value but server is not yet support

        simple_resp::redis_type_value_pair_list result;
        for(int i = 1; i < redis_args.size(); i++)
        {
            simple_resp::redis_type_value_pair one_result_pair;
            try{
                auto one_result = getTree()->readEx(redis_args[i]);
                one_result_pair.type = simple_resp::SIMPLE_STRINGS;
                one_result_pair.value = one_result;
            } catch (const LogCabin::Client::LookupException& e) {
                one_result_pair.type = simple_resp::BULK_NIL;
                one_result_pair.value = "";
            }
            result.push_back(one_result_pair);
        }
        auto encodeResult = enc.encode(result);

        return encodeResult;
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

encode_result
handler::handle_smembers_request(const std::vector<std::string>& redis_args)
{
    try {
        if (redis_args.size() != 2) {
            return enc.encode(simple_resp::ERRORS, {"ERR wrong number of arguments for 'smembers' command"});
        }
        //FIXME: need to detect return value but server is not yet support
        auto result = getTree()->smembers(redis_args[1]);


        auto encodeResult = enc.encode(simple_resp::ARRAYS, result);

        return encodeResult;
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
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
        auto result = getTree()->lrange(redis_args[1], redis_args[2] + " " + redis_args[3]);


        auto encodeResult = enc.encode(simple_resp::ARRAYS, result);

        return encodeResult;
    } catch (const LogCabin::Client::LookupException& e) {
        return enc.encode(simple_resp::BULK_NIL, {""});
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
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

        result = getTree()->ltrim(redis_args[1], redis_args[2] + " " + redis_args[3]);

        if (result.status == Status::OK) {
            return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
        } else if (result.status == Status::INVALID_ARGUMENT) {
            return enc.encode(simple_resp::ERRORS, {"ERR invalid arguments for 'ltrim' command"});
        } else {
            return enc.encode(simple_resp::ERRORS, {"ERR unknown error for 'ltrim' command"});
        }
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what()
                  ;
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

        result = getTree()->expire(redis_args[1], std::atoi(redis_args[2].c_str()));

        if (result.status == Status::OK) {
            return enc.encode(simple_resp::SIMPLE_STRINGS, {"OK"});
        } else if (result.status == Status::INVALID_ARGUMENT) {
            return enc.encode(simple_resp::ERRORS, {"ERR invalid arguments for 'expire' command"});
        } else {
            return enc.encode(simple_resp::ERRORS, {"ERR unknown error for 'expire' command"});
        }
    } catch (const LogCabin::Client::Exception& e) {
        LOG(ERROR) << "Exiting due to LogCabin::Client::Exception: "
                  << e.what();
        return enc.encode(simple_resp::ERRORS, {"ERR Internal error happened"});
    }
}

//TODO: need to refractor such ugly code
std::vector<std::string>
handler::split_list_elements(const std::string& original) {
    std::vector<std::string> elements;
    if (original.length() == 0) {
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
