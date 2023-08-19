#include "./proxy/ProxyService.h"
#include "./rpc/RpcChannel.h"

ProxyService::ProxyService(const char* ip , const uint16_t port) :
                                ip_(ip),
                                port_(port),
                                master_("/ChatService"),
                                user_stub_(new RpcChannel())
{
    master_.Start() ; 
    msg_handler_map_.insert({"Login", std::bind(&ProxyService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"Regist", std::bind(&ProxyService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"Logout", std::bind(&ProxyService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
}

// ProxyService 中的所有方法都是只充当一个转发的作用

//登录
void ProxyService::login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    ik_UserService::LoginRequest login_request;
    login_request.ParseFromString(recv_buf);
    login_request.set_ip_port(std::string(ip_) + ":" + std::to_string(port_)) ;

    //执行
    ik_UserService::LoginReponse response;
    user_stub_.Login(nullptr, &login_request, &response, nullptr);
 
    if(response.is_success())
    { 
        //添加此用户到user map表中
        {
            std::lock_guard<std::mutex> lock(mutex_);
            use_connection_map_.insert({login_request.id(), conn});
        }
    }
    //序列化并发送
    std::string send_str = response.SerializeAsString();
    conn->send(send_str);
}

//注册 , 注册完需要重新登录
void ProxyService::regist(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    ik_UserService::RegisterRequest regist_request;
    regist_request.ParseFromString(recv_buf);

    //执行
    ik_UserService::RegisterResponse response;
    user_stub_.Registe(nullptr, &regist_request, &response, nullptr);

    //序列化并发送
    std::string send_str = response.SerializeAsString();
    conn->send(send_str);
}

//注销
void ProxyService::logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    ik_UserService::LogOutRequest request;
    request.ParseFromString(recv_buf);

    //执行
    ik_UserService::LogOutResponse response;
    user_stub_.LoginOut(nullptr, &request, &response, nullptr);

    if(response.is_success())
    { 
        //删除此用户到user map表中
        {
            std::lock_guard<std::mutex> lock(mutex_);
            use_connection_map_.erase(request.id());
        }
    }

    //序列化并发送
    std::string send_str = response.SerializeAsString();
    conn->send(send_str);
}

//获得消息对应的处理器
ProxyService::MsgHandler ProxyService::get_handler(std::string msg_type)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = msg_handler_map_.find(msg_type);
    //如果没有对应的msgid
    if (it == msg_handler_map_.end())
    {
        return [=](const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time) {};
    }
    else
    {
        return msg_handler_map_[msg_type];
    }
}

//处理客户端异常退出
void ProxyService::client_close_exception(const muduo::net::TcpConnectionPtr &conn)
{
    //1. 线程安全 2. 删除user_map 3.用户改为offline 
    int userId = -1 ; 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = use_connection_map_.begin(); it != use_connection_map_.end(); it++)
        {
            if (it->second == conn)
            {
                userId = it->first ;
                // redis_client_.del_host(it->first);
                it = use_connection_map_.erase(it);
                break;
            }
        }
    }

    if(userId != -1) {
        ik_UserService::LogOutRequest request;
        request.set_id(userId) ;
        ik_UserService::LogOutResponse response;
        user_stub_.LoginOut(nullptr, &request, &response, nullptr);
    }
}

// 服务端异常，清理客户端资源
void ProxyService::reset()
{
    //序列化客户端收到的语句
    Proxy::ProxyResponse response;
    response.set_type("LoginOut");
    response.set_response_msg("server crash");
    std::string send_str = response.SerializeAsString();
    std::vector<int> userIds ; 
    // 减少锁粒度，另外存储 userIds 发送 Login
    {
        std::lock_guard<std::mutex> lock(mutex_);
        //重置所有用户为下线状态 
        for (auto it = use_connection_map_.begin(); it != use_connection_map_.end(); it++)
        {
            userIds.push_back(it->first) ; 
            it->second->send(send_str);
        }
        // 清空map
        use_connection_map_.clear();
    }
    
    // 远程 RPC 调用，可能会更加费时间
    ik_UserService::LogOutRequest request;
    for(int id : userIds) 
    {
        request.set_id(id) ;
        ik_UserService::LogOutResponse response;
        user_stub_.LoginOut(nullptr, &request, &response, nullptr);
    }
}