#ifndef SESSIONMANAGER_H_7CFDHMLI
#define SESSIONMANAGER_H_7CFDHMLI

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "simple_resp.h"
#include <mutex>
#include <map>

class SessionCtx{
    public:
        SessionCtx(int fd, std::function<void (std::vector<std::string>&)> callback):
            fd(fd), is_eof_reach(false), ctx(callback){};
        int fd;
        bool is_eof_reach;
        simple_resp::decode_context ctx;

};

class SessionManager
{
private:
    static SessionManager* instance;
    int session_counter;
    std::mutex session_counter_mutex;
    std::map<int, SessionCtx> ctx_map;

    int get_next_session_counter_number();

public:
    SessionManager(){};
    virtual ~SessionManager(){};

    SessionCtx* get_ctx(int session_id);
    void session_get_eof(int session_id);
    int insert_new_staus(int fd, std::function<void (std::vector<std::string>&)> callback);
    void erase_session(int sesion_id);

    static SessionManager* getInstance();
};

#endif /* SESSIONMANAGER_H */

#endif /* end of include guard: SESSIONMANAGER_H_7CFDHMLI */
