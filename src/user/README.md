## User 集群的设计
![](../../image/userFollower.jpg)
我们对外提供一个主节点 UserMaster ，所有关于登录、注册、注销的业务需求都交由 UserMaster 转发给 UserFollower 处理，从而实现集群的负载均衡能力。UserFollower 是真正负责处理请求响应的节点，它负责与 Redis、Mysql 进行交互，并且将自己的服务器信息注册到 Zookeeper 上以便供 UserMaster 获取和转发请求。

### 主节点：UserMaster
所有关于 登录、注册、注销、的业务请求都会在这里得到中转，按照轮询的方式每次都选取一个可用的 UserFollower 节点处理请求，然后接收 UserFollower 的请求转发给回去，如 Login 请求
```
void UserMaster::Login(::google::protobuf::RpcController *controller,
               const ::User::LoginRequest *request,
               ::User::LoginReponse *response,
               ::google::protobuf::Closure *done)
{
    // test 
    // std::cout << request->id() << " " << request->password() << " " << request->ip_port() << " " << std::endl ;
    std::cout << "Login Method" << std::endl ;
    //获取一个子节点信息
    std::shared_ptr<Socket> client_fd;
    while ((client_fd = zkClient_.GetFollowerFd()) == nullptr)
    {
        sleep(1);
    } 
    User::UserRequest userRequest ;  
    userRequest.set_type("Login") ; 
    userRequest.set_message(request->SerializeAsString()) ; 

    std::string send_str = userRequest.SerializeAsString() ;

    //发送信息
    send(client_fd->fd(), send_str.c_str(), send_str.size(), 0);

    //获取信息
    char recv_buf[256] = {0};
    recv(client_fd->fd(), recv_buf, 256, 0);
    if (response->ParseFromString(recv_buf) == false)
    {
        std::cout << "userMaster Parse userFollower Login response Fail" << std::endl ;
    } 
    // std::cout << "result = " << response->is_success() << " msg = " << response->message() << std::endl ;
    done->Run();
}

```

### 从节点：UserFollower
UserFollower 是整个服务集群中真正提供请求响应处理的节点，提供了登录、注册、注销三个功能。当一个业务请求通过 UserMaster 分发到 UserFollower 上的时候，它会先去反序列化请求，根据具体的请求类型进行业务处理，并返回响应的结果 , 如识别该处理那个请求的响应。
```C++
// 可读事件到来之后的回调函数
void UserFollower::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp stamp)
{
    //反序列化由 UserMaster 传过来的序列化数据
    std::string message = buffer->retrieveAllAsString();
    User::UserRequest request;
    request.ParseFromString(message);
    std::string methodType = request.type() ; 
    std::string responseMessage ; 
    //登录业务
    if (methodType == "Login")
    {
        // 反序列化数据得到 id、password
        User::LoginRequest login_request;
        login_request.ParseFromString(request.message()); 
        // 执行具体的Login方法，执行成功 is_success 被设置为true
        User::LoginReponse login_response;
        std::cout << "Login UserFollower : " << login_request.name() << " " << login_request.password() << " " << login_request.ip_port() << std::endl ;
        int ret = Login(login_request.name() , login_request.password() , login_request.ip_port()) ;
        if (ret == 1) {
            login_response.set_is_success(true);
            login_response.set_message("login success");
        }
        else if(ret == -1) {
            login_response.set_is_success(false); 
            login_response.set_message("password error");
        }else if(ret == 0) {
            login_response.set_is_success(false); 
            login_response.set_message("user already login");
        }
        //序列化结果 并将结果返回
        responseMessage = login_response.SerializeAsString();
    }
    else if (methodType == "Register")  //注册业务
    {
        User::RegisterRequest register_request;
        register_request.ParseFromString(request.message()); 
        User::RegisterResponse register_response; 
        std::cout << "Register UserFollower : " << register_request.name() << " " << register_request.password()  << std::endl ;
        if (Register(register_request.name(), register_request.password()) == false)
        {
            register_response.set_is_success(false);
            register_response.set_message("Register fail , user already exist") ;
        }
        else
        {
            register_response.set_is_success(true) ;
            register_response.set_message("Register success") ;
        }
        responseMessage = register_response.SerializeAsString(); 
    }
    else if (methodType == "LogOut")  //注销业务
    {
        User::LogOutRequest logout_request;
        logout_request.ParseFromString(request.message());
        std::string name = logout_request.name();
        std::cout << "LogOut UserFollower : " << logout_request.name() << " " << logout_request.ip_port()  << std::endl ;
        User::LogOutResponse logout_response; 
        if (LogOut(logout_request.name() , logout_request.ip_port()) == false)
        {
            logout_response.set_is_success(false);
            logout_response.set_message("Logout fail") ;
        }
        else
        {
            logout_response.set_is_success(true) ;
            logout_response.set_message("Logout success") ;
        }
        responseMessage = logout_response.SerializeAsString(); 
    } 
    else if (methodType == "GetUserInfo")  //注销业务
    {
        std::cout << "GetUserInfo UserFollower " << std::endl ;
        User::GetUserInfoResponse userNameResponse ;
        std::vector<std::string> userNames = GetUserInfo() ; 
        if(userNames.size() > 0) {
            userNameResponse.set_is_success(true) ; 
            for(auto name : userNames) {
                userNameResponse.add_usernames(name) ;
            }
        }else {
            userNameResponse.set_is_success(false) ; 
        }
        responseMessage = userNameResponse.SerializeAsString(); 
    } 
    else {
        std::cout << "invaild request : " << methodType << std::endl ; 
    }

    // 这里其实还需要判断发送是否成功等
    conn->send(responseMessage.data(), responseMessage.size()) ; 
}
```

示例处理 Login 请求的代码：

```C++
//登录
int UserFollower::Login(const std::string& name, const std::string &password , const std::string &proxyIP)
{
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();

    char sql[64] = {0};
    sprintf(sql, "select id from user where name='%s' and password='%s'", name.data(), password.data()); 
    if (conn->query(sql) && conn->next()) // 账号密码正确
    {
        conn->freeResult() ; 
        // 默认登录状态保存 5 小时
        if(redisClient_.redisLock(name , proxyIP , std::to_string(5*60*60*1000) ) == false) 
        {
            return 0 ; // 已经在其他地方登录了 
        }
        return 1 ;
    } 
    return -1 ; // 账号密码错误 
}
```
