#include "SessionManager.h"

#include "glog/logging.h"
#include <unistd.h>

SessionCtx::~SessionCtx(){
    DLOG(INFO) << "~SessionCtx";
}

SessionCtx::SessionCtx(int fd):
    fd(fd), to_send_command_id(0), is_eof_reach(false){
        DLOG(INFO) << "in init command the send command_id is:" << to_send_command_id << " and the this ptr is:" << this;
        buffered_responses.clear();
    };

static int
reply(int fd, std::string &content)
{
    int sent_size = 0;
    while(sent_size < content.length())
    {
        int one_send_size = write(fd, content.c_str() + sent_size, content.length() - sent_size);
        if(0 <= one_send_size){
            return one_send_size;
        }
        sent_size += one_send_size;
    }
    return 1;
}

void SessionCtx::insert_new_response(int command_id, std::string& resp)
{
    std::lock_guard<std::mutex> guard(response_map_mutex);
    buffered_responses.insert(std::make_pair(command_id, resp));
    auto it = buffered_responses.begin();
    DLOG(INFO) << "this ptr:"<< this;
    DLOG(INFO) << "the command_id is:" << command_id << " and current to send id is " << to_send_command_id;
    if(command_id == to_send_command_id)
    {
        send_first_response();
    }
}

void SessionCtx::send_first_response()
{
    auto it = buffered_responses.begin();
    int reply_result = 1;
    while(buffered_responses.end() != it && it->first == to_send_command_id)
    {
        reply(this->fd, it->second);
        if(reply_result <= 0 )
        {
            break;
        }
        to_send_command_id++;
        DLOG(INFO) << "after update, to command id:" << to_send_command_id;
        buffered_responses.erase(it++);
    }
    if(reply_result <=0)
    {
        this->is_eof_reach = true;
    }
}

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
    std::lock_guard<std::mutex> guard(session_map_mutex);
    auto new_session_ctx_ptr = std::make_shared<SessionCtx>(fd);
    DLOG(INFO) << "the ctx ptr is:" << new_session_ctx_ptr;
    ctx_map.insert(std::make_pair(session_id, new_session_ctx_ptr));
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
    std::lock_guard<std::mutex> guard(session_map_mutex);
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() != it)
    {
        int fd = it->second->fd;
        close(it->second->fd);
        it->second->is_eof_reach = true;
        ctx_map.erase(it);
    }
}

int SessionManager::get_next_session_counter_number()
{
    std::lock_guard<std::mutex> guard(session_counter_mutex);
    session_counter++;
    return session_counter;
}

std::shared_ptr<SessionCtx> SessionManager::get_ctx(int session_id)
{
    auto it = ctx_map.find(session_id);
    if(ctx_map.end() == it){
        return NULL;
    }else{
        return it->second;
    }
}
