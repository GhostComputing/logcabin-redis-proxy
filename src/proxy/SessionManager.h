#ifndef SESSIONMANAGER_H_7CFDHMLI
#define SESSIONMANAGER_H_7CFDHMLI

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "simple_resp.h"
#include <mutex>
#include <map>
#include <memory>

class SessionCtx{
    private:
        void erase_first_response();
        void send_first_response();
        int to_send_command_id;
        int fd;
        std::map<int, std::string> buffered_responses;
        std::mutex response_map_mutex;
    public:
        SessionCtx(int fd);
        ~SessionCtx();
        //just insert, we will handle the left things
        void insert_new_response(int command_id, std::string& resp);

        const std::string& get_first_response();


        bool is_eof_reach;

        friend class SessionManager;
};

class SessionManager
{
private:
    static SessionManager* instance;
    int session_counter;
    std::mutex session_counter_mutex;

    std::mutex session_map_mutex;
    std::map<int, std::shared_ptr<SessionCtx> > ctx_map;

    int get_next_session_counter_number();

public:
    SessionManager(){};
    virtual ~SessionManager(){};

    std::shared_ptr<SessionCtx> get_ctx(int session_id);
    void session_get_eof(int session_id);
    int insert_new_staus(int fd);
    void erase_session(int sesion_id);

    static SessionManager* getInstance();
};

#endif /* SESSIONMANAGER_H */

#endif /* end of include guard: SESSIONMANAGER_H_7CFDHMLI */
