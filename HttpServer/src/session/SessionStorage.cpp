#include "../include/session/SessionStorage.h"
#include <iostream>
#include <muduo/base/Logging.h>
namespace http
{

namespace session
{

void MemorySessionStorage::save(std::shared_ptr<Session> session)
{
    // 创建会话副本并存储
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->getId()] = session;
    std::cout << "Session " << session->getId() << " saved to memory storage Success." << std::endl;
}

// 通过会话ID从存储中加载会话
std::shared_ptr<Session> MemorySessionStorage::load(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end())
    {
        if (!it->second->isExpired())
        {
            std::cout << "Session " << sessionId << " loaded from memory storage Success" << std::endl;
            return it->second;
        }
        else
        {
            // 如果会话已过期，则从存储中移除
            std::cout << "Session " << sessionId << " expired, loaded from memory storage Failed" << std::endl;
            sessions_.erase(it);
        }
    }else{
        std::cout << "Session " << sessionId << " don`t exited, loaded from memory storage Failed" << std::endl;
    }

    // 如果会话不存在或已过期，则返回nullptr
    return nullptr;
}

// 通过会话ID从存储中移除会话
void MemorySessionStorage::remove(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

void MemorySessionStorage::cleanExpiredSession()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        if(it != sessions_.end() && it->second){
            if (it->second->isExpired())
            {
                std::cout << "clean expired session: " << it->second->getId() << std::endl;
                it = sessions_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

} // namespace session
} // namespace http