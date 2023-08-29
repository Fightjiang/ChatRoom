#ifndef PROXY_SERVICE_H
#define PROXY_SERVICE_H

#include "../proxy/Proxy.pb.h"
#include "../user/User.pb.h"
#include "../chat/ChatMessage.pb.h"
#include "../base/ZookeeperUtil.h"
#include "../base/Redis.h"

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h> 
#include <muduo/base/Logging.h>
#include <mutex>
#include <unordered_map>
#include <functional>

// 为什么这个转发类采用这种函数 map 搜索的方式呢？
// 主要是因为要与客户建立连接的 TcpConnectionPtr 不能封装在框架内
// 还需要通过 Send 发送消息给其他的客户端

class ProxyService
{
public :

    ProxyService(const char* ip = "127.0.0.1" , const uint16_t port = 8001);

    //登录
    void login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    //注册
    void regist(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    //注销业务
    void logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    
    // 获得用户信息
    void getUserInfo(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    // 客户端发送过来的聊天信息处理
    void chatMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    
    // ChatFollower 发送过来的聊天信息处理
    void forwardMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    // 刚登录需要读取该用户的离线消息
    void readOfflineMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    //处理客户端异常退出
    void clientCloseException(const muduo::net::TcpConnectionPtr &conn);

    // 服务器异常，清理客户端资源
    void reset() ; 

    using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)>;

    //获得消息对应的处理器
    MsgHandler getHandler(std::string msg_type);

private:

    ZkClient master_;       //连接zookeeper服务器

    std::unordered_map<std::string, MsgHandler> msg_handler_map_;              //存储事件及事件对应的回调函数
    std::unordered_map<std::string, muduo::net::TcpConnectionPtr> use_connection_map_; //存储登录用户及对应 文件描述符信息 便于发送消息给客户端
    std::mutex mutex_;    

private:
    User::UserServiceRpc_Stub user_stub_ ;
    const char* ip_ ; 
    const uint16_t port_ ;  
};

#endif