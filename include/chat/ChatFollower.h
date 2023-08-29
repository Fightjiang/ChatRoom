#ifndef CHAT_FOLLOWER_H
#define CHAT_FOLLOWER_H

#include "../chat/ChatMessage.pb.h"
#include "../proxy/Proxy.pb.h"
#include "../base/MysqlConnectionPool.h"
#include "../base/ZookeeperUtil.h"
#include "../base/Redis.h"
#include "../base/Socket.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Logging.h>
#include <functional>
#include <string>
#include <unordered_map>

class ChatFollower
{
public:
    ChatFollower() : MysqlPool_(ConnectionPool::getConnectionPool()) {}

    //服务启动
    void run(const char * ip , const size_t port);

    //读写事件回调函数
    void on_message(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp stamp);

    //连接事件回调函数
    void on_connetion(const muduo::net::TcpConnectionPtr& conn);

private: 
    
    //根据 Redis 中的 host 与 Proxy 建立连接
    std::shared_ptr<Socket> establish_connection(const std::string& host) ;
    
    // 发送消息
    void sendChatMessage(const std::string& recvStr) ; 
    
    // 写入离线消息
    bool writeOffline(const std::string& sendName , 
                      const std::string& recvName , 
                      const std::string& message , const int flag = 0) ; 
    // 读入离线消息
    std::vector<std::string> readOffline(const std::string& Name , 
                     ChatMessage::ReadOfflineResponse *response) ;
    
    // 伪删除该用户的离线消息，其实是修改数据状态
    bool deleteOffline(const std::vector<std::string> & messageIDs) ;

    std::string queryNameId(const std::string &name) ;

    //存储 chatFollower 与 Proxy 建立的连接描述符，避免频繁建立连接所带来的开销
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Socket>> channel_map_;

    ConnectionPool* MysqlPool_;  //数据库连接池
    muduo::net::EventLoop loop_;
    RedisCli redisClient_ ; 
    ZkClient zkClient_;
};

#endif
