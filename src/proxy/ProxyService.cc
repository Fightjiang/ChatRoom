#include "./proxy/ProxyService.h"
#include "./rpc/RpcChannel.h"

ProxyService::ProxyService(const char* ip , const uint16_t port) :
                                ip_(ip),
                                port_(port),
                                master_("/ChatMaster"),
                                user_stub_(new RpcChannel())
{
    msg_handler_map_.insert({"Login", std::bind(&ProxyService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"Register", std::bind(&ProxyService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"Logout", std::bind(&ProxyService::logout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"getUserInfo", std::bind(&ProxyService::getUserInfo, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"chatMessage", std::bind(&ProxyService::chatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msg_handler_map_.insert({"forwardMessage", std::bind(&ProxyService::forwardMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
}

// ProxyService 中的所有方法都是只充当一个转发的作用

//登录
void ProxyService::login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    User::LoginRequest login_request;
    login_request.ParseFromString(recv_buf);
    login_request.set_ip_port(std::string(ip_) + ":" + std::to_string(port_)) ;

    // std::cout << " id = " << login_request.name() << " password = " << login_request.password() 
    //           << " ip_port " << login_request.ip_port() << std::endl ;
    //执行
    User::LoginReponse response;
    user_stub_.Login(nullptr, &login_request, &response, nullptr);
    std::cout << "result = " << response.is_success() << " msg = " << response.message() << std::endl ;
    if(response.is_success())
    { 
        //添加此用户到user map表中
        {
            std::lock_guard<std::mutex> lock(mutex_);
            use_connection_map_.insert({login_request.name(), conn});
        }
    }
    //序列化并发送
    Proxy::ProxyResponse ProxyResponse;
    ProxyResponse.set_type("Login") ;
    ProxyResponse.set_response_msg(response.SerializeAsString()) ;
    
    std::string send_str = ProxyResponse.SerializeAsString();
    conn->send(send_str);
}

//注册 , 注册完需要重新登录
void ProxyService::regist(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    User::RegisterRequest regist_request;
    regist_request.ParseFromString(recv_buf);

    //执行
    User::RegisterResponse response;
    user_stub_.Register(nullptr, &regist_request, &response, nullptr);

    //序列化并发送
    Proxy::ProxyResponse ProxyResponse;
    ProxyResponse.set_type("Register") ;
    ProxyResponse.set_response_msg(response.SerializeAsString()) ;
    
    std::string send_str = ProxyResponse.SerializeAsString();
    conn->send(send_str);
}

//注销
void ProxyService::logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    User::LogOutRequest request;
    request.ParseFromString(recv_buf);
    request.set_ip_port(std::string(ip_) + ":" + std::to_string(port_)) ;
    //执行
    User::LogOutResponse response;
    user_stub_.LogOut(nullptr, &request, &response, nullptr);

    if(response.is_success())
    { 
        //删除此用户到user map表中
        {
            std::lock_guard<std::mutex> lock(mutex_);
            use_connection_map_.erase(request.name());
        }
    }

    //序列化并发送
    Proxy::ProxyResponse ProxyResponse;
    ProxyResponse.set_type("Logout") ;
    ProxyResponse.set_response_msg(response.SerializeAsString()) ;
    
    std::string send_str = ProxyResponse.SerializeAsString();
    conn->send(send_str); 
}
// 获得用户的信息
void ProxyService::getUserInfo(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    User::UserRequest request; 
    request.set_type("GetUserInfo") ;
    //执行
    User::GetUserInfoResponse response;
    user_stub_.GetUserInfo(nullptr, &request, &response, nullptr);

    //序列化并发送
    Proxy::ProxyResponse ProxyResponse;
    ProxyResponse.set_type("GetUserInfo") ;
    ProxyResponse.set_response_msg(response.SerializeAsString()) ;
    
    std::string send_str = ProxyResponse.SerializeAsString();
    conn->send(send_str); 
     
}

// 收到客户端发送给其他用户的聊天消息
void ProxyService::chatMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 反序列化
    ChatMessage::Message request;
    request.ParseFromString(recv_buf);

    std::string recvName = request.recvname() ; 
    auto it = use_connection_map_.find(recvName);

    std::cout <<"sendname : "<< request.sendname() << " recvname :" << request.recvname() 
              << " msg: " << request.msg() << std::endl ;

    // 不在此服务器上
    if (it == use_connection_map_.end())
    {
        // 获取一个与转发服务器交互的可以连接
        std::shared_ptr<Socket> client_fd;
        while ((client_fd = master_.GetFollowerFd()) == nullptr)
        {
            sleep(1);
        }
        // 发给 chatFollower 节点服务器做消息转发
        // chatFollower 通过查找 Redis 查看 recvName 用户在那台 Proxy 节点登录
        // 再与 Proxy 节点建立联系，发送转发消息给 Proxy 节点转发给客户端

        ChatMessage::ChatRequest chatRequest ;
        chatRequest.set_type("ChatMessage") ;
        chatRequest.set_message(recv_buf) ;
        std::string sendStr = chatRequest.SerializeAsString() ; 
        if(send(client_fd->fd(), sendStr.data(), sendStr.size(), 0) == -1) {
            ChatMessage::ChatResponse chatResponse ; 
            chatResponse.set_is_success(false) ; 
            chatResponse.set_message("send chat Message request error") ; 
            
            //序列化并发送
            Proxy::ProxyResponse ProxyResponse;
            ProxyResponse.set_type("SendChatMessage") ;
            ProxyResponse.set_response_msg(chatResponse.SerializeAsString()) ;
            
            std::string send_str = ProxyResponse.SerializeAsString();
            conn->send(send_str); 
             
            return ; 
        }
    }
    else// 客户端与该 Proxy 服务器直接相连着的，则直接转发消息给该用户
    {
        //序列化并发送
        Proxy::ProxyResponse ProxyResponse;
        ProxyResponse.set_type("RecvChatMessage") ;
        ProxyResponse.set_response_msg(recv_buf) ;
        std::string send_str = ProxyResponse.SerializeAsString();
        it->second->send(send_str);
    }
    
    ChatMessage::ChatResponse chatResponse ;  
    chatResponse.set_is_success(true) ; 
    chatResponse.set_message("send chat Message request success") ; 
    
    Proxy::ProxyResponse ProxyResponse;
    ProxyResponse.set_type("SendChatMessage") ;
    ProxyResponse.set_response_msg(chatResponse.SerializeAsString()) ;

    //序列化并发送
    std::string send_str = ProxyResponse.SerializeAsString();
    conn->send(send_str);  
}

// 收到 ChatFollower 发送过来的转发消息给客户端的请求
void ProxyService::forwardMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    //反序列化
    ChatMessage::Message request;
    request.ParseFromString(recv_buf);
    
    // 转发消息给用户 response.set_type 用于给用户区分类型
    Proxy::ProxyResponse response;
    response.set_type("RecvChatMessage");
    response.set_response_msg(recv_buf);

    std::string recvName  = request.recvname();
    muduo::net::TcpConnectionPtr client_conn = nullptr ;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = use_connection_map_.find(recvName);
        client_conn = it->second;
    }

    if (client_conn != nullptr) {
        client_conn->send(response.SerializeAsString());
    } else { // 处理转发的时候用户下线这种异常的情况， send 失败则让 chatFollower 继续写入离线消息表中 
        std::cout << "user already LogOut" << std::endl ; 
        // 获取一个与转发服务器交互的可以连接
        std::shared_ptr<Socket> chat_fd;
        while ((chat_fd = master_.GetFollowerFd()) == nullptr)
        {
            sleep(1);
        }
        
        ChatMessage::ChatRequest chatRequest ;
        chatRequest.set_type("WriteOffline") ;
        chatRequest.set_message(recv_buf) ;
        std::string sendStr = chatRequest.SerializeAsString() ; 
        send(chat_fd->fd(), sendStr.data(), sendStr.size(), 0);
    } 
}

// 刚登录需要读取该用户的离线消息
void ProxyService::readOfflineMessage(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    
    // 获取一个与转发服务器交互的可以连接
    std::shared_ptr<Socket> client_fd;
    while ((client_fd = master_.GetFollowerFd()) == nullptr)
    {
        sleep(1);
    }

    // 发给 chatFollower 节点服务器要求读取 Mysql 中存储的离线消息
    // 再转发消息给给客户端

    ChatMessage::ChatRequest chatRequest ;
    chatRequest.set_type("ReadOffline") ;
    chatRequest.set_message(recv_buf) ;
    std::string sendStr = chatRequest.SerializeAsString() ; 

    // 发送请求
    send(client_fd->fd(), sendStr.data(), sendStr.size(), 0); 

    //获取信息
    char response_buf[1024] = {0} ;
    recv(client_fd->fd(), response_buf, 1024, 0) ; 
    
    Proxy::ProxyResponse response ; 
    response.set_type("ReadOffline") ; 
    response.set_response_msg(response_buf) ; 
    conn->send(response.SerializeAsString()) ;
}


//获得消息对应的处理器
ProxyService::MsgHandler ProxyService::getHandler(std::string msg_type)
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
void ProxyService::clientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    //1. 线程安全 2. 删除user_map 3.用户改为offline 
    std::string userName ; 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = use_connection_map_.begin(); it != use_connection_map_.end(); it++)
        {
            if (it->second == conn)
            {
                userName = it->first ;
                // redis_client_.del_host(it->first);
                it = use_connection_map_.erase(it);
                break;
            }
        }
    }

    if(userName.size() > 0 ) {
        User::LogOutRequest request;
        request.set_name(userName) ;
        request.set_ip_port(std::string(ip_) + ":" + std::to_string(port_)) ;
        User::LogOutResponse response;
        user_stub_.LogOut(nullptr, &request, &response, nullptr);
    }
}

// 服务端异常，清理客户端资源
void ProxyService::reset()
{
    //序列化客户端收到的语句
    Proxy::ProxyResponse response;
    response.set_type("LogOut");
    response.set_response_msg("server crash");
    std::string send_str = response.SerializeAsString();
    std::vector<std::string> userNames ; 
    // 减少锁粒度，另外存储 userIds 发送 Login
    {
        std::lock_guard<std::mutex> lock(mutex_);
        //重置所有用户为下线状态 
        for (auto it = use_connection_map_.begin(); it != use_connection_map_.end(); it++)
        {
            userNames.push_back(it->first) ; 
            it->second->send(send_str);
        }
        // 清空map
        use_connection_map_.clear();
    }
    
    // 远程 RPC 调用，可能会更加费时间
    User::LogOutRequest request;
    std::string ipPort = std::string(ip_) + ":" + std::to_string(port_) ; 
    for(const std::string &name : userNames) 
    {
        request.set_name(name) ;
        request.set_name(ipPort) ;
        User::LogOutResponse response;
        user_stub_.LogOut(nullptr, &request, &response, nullptr);
    }
}