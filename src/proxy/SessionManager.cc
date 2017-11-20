#include "SessionManager.h"

#include <unistd.h>

SessionManager* SessionManager::instance = NULL;

SessionManager* SessionManager::getInstance(){
    if(NULL == SessionManager::instance){
        SessionManager::instance = new SessionManager();
    }
    return SessionManager::instance;
}

int SessionManager::insert_new_staus(int fd, std::function<void (std::vector<std::string>&)> callback)
{
    int session_id = get_next_session_counter_number();
    ctx_map.insert(std::make_pair(session_id, SessionCtx(fd, callback)));
    return session_id;
}

void SessionManager::session_get_eof(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() == it)
    {
        return;
    }
    it->second.is_eof_reach = true;
}

void SessionManager::erase_session(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() != it)
    {
        ctx_map.erase(it);
        close(it->second.fd);
    }
}


int SessionManager::get_next_session_counter_number()
{
    std::lock_guard<std::mutex> guard(session_counter_mutex);
    session_counter++;
    return session_counter;
}

SessionCtx* SessionManager::get_ctx(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() == it){
        return NULL;
    }else{
        return &it->second;
    }
}
