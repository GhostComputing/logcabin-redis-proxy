#include "SessionManager.h"

#include <unistd.h>

SessionManager* SessionManager::instance = NULL;

SessionManager* SessionManager::getInstance(){
    if(NULL == SessionManager::instance){
        SessionManager::instance = new SessionManager();
    }
    return SessionManager::instance;
}

int SessionManager::insert_new_staus(int fd)
{
    int session_id = get_next_session_counter_number();
    ctx_map.insert(std::make_pair(session_id, new SessionCtx(fd)));
    fd_map.insert(std::make_pair(fd, session_id));
    return session_id;
}

void SessionManager::session_get_eof(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() == it)
    {
        return;
    }
    it->second->is_eof_reach = true;
}

void SessionManager::erase_session(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() != it)
    {
        int fd = it->second->fd;
        auto fd_it = fd_map.find(fd);
        if(fd_map.end() != fd_it)
        {
            fd_map.erase(fd_it);
        }
        close(it->second->fd);
        delete it->second;
        ctx_map.erase(it);
    }
}

void SessionManager::erase_session_by_fd(int fd)
{
    auto it = fd_map.find(fd);
    if(fd_map.end() != it)
    {
        int session_id = it->second;
        fd_map.erase(it);
        auto session_it = ctx_map.find(session_id);
        if(ctx_map.end() != session_it)
        {
            close(session_it->second->fd);
            delete session_it->second;
            ctx_map.erase(session_id);
        }
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
        return it->second;
    }
}
