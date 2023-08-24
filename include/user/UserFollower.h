#ifndef USER_FOLLOWER_H
#define USER_FOLLOWER_H

#include "../user/User.pb.h"
#include "../base/MysqlConnectionPool.h"
#include "../base/ZookeeperUtil.h"
#include "../base/Redis.h"

#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Logging.h>
#include <functional>
#include <string>

class UserFollower
{
public:
    UserFollower() : MysqlPool_(ConnectionPool::getConnectionPool()) {}

    //服务启动
    void run(const char * ip , const size_t port);

    //读写事件回调函数
    void on_message(const muduo::net::TcpConnectionPtr& conn,muduo::net::Buffer* buffer,muduo::Timestamp stamp);

    //连接事件回调函数
    void on_connetion(const muduo::net::TcpConnectionPtr& conn);

public:
    //登录
    int Login(const std::string& name, const std::string &password , const std::string &proxyIP);

    //注销
    bool LogOut(const std::string &name , const std::string &proxyIP);

    //注册 成功返回注册的账户，失败返回-1
    bool Register(const std::string& name, const std::string& password);

public: 

    ConnectionPool* MysqlPool_;  //数据库连接池
    muduo::net::EventLoop loop_;
    RedisCli redisClient_ ; 
    ZkClient zkClient_;
};


#endif
