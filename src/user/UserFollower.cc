#include "./user/UserFollower.h"

void UserFollower::run(const char * ip , const size_t port)
{
    //初始化网络层
    muduo::net::InetAddress address(ip , port);
    muduo::net::TcpServer server(&loop_, address, "UserFollower");

    server.setMessageCallback(std::bind(&UserFollower::on_message, this, 
                               std::placeholders::_1, 
                               std::placeholders::_2, 
                               std::placeholders::_3));

    server.setConnectionCallback(std::bind(&UserFollower::on_connetion, this, 
                                 std::placeholders::_1));


    // 把自己的节点信息注册到 zookeeper 上，供主节点查找和发现 , 临时顺序节点
    std::string server_path = "/UserMaster/server";
    char nodeValue[64] = {0};
    sprintf(nodeValue, "%s:%lu", ip , port);
    if (zkClient_.CreateNode(server_path.data(), nodeValue, strlen(nodeValue), ZOO_EPHEMERAL | ZOO_SEQUENCE) == false)
    {
        std::cout << "zookeeper create ZOO_SEQUENCE node fail path = " << server_path << " data = " << nodeValue << std::endl ;
        return ; 
    } 
    //开启事件循环
    std::cout << "UserFollower ip: " << ip <<" port: " << port <<" run " << std::endl ;
    server.setThreadNum(4);
    server.start();
    loop_.loop();
}

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

//连接事件回调函数
void UserFollower::on_connetion(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client断开连接
        conn->shutdown();
    }
}

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

// 注销
bool UserFollower::LogOut(const std::string &name , const std::string &proxyIP)
{
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection() ;
    return redisClient_.redisUnLock(name , proxyIP) ; 
}

// 注册
bool UserFollower::Register(const std::string& name, const std::string& password)
{
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection() ;
    char sql[64] = {0};
    sprintf(sql, "insert into user(name,password) values('%s','%s')", name.data(), password.data());
    return conn->update(sql);
}

std::vector<std::string> UserFollower::GetUserInfo()
{
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection() ;
    std::string sql = "select name from user" ; 
    std::vector<std::string> names ; 
    if (conn->query(sql) && conn->next())  
    {
        do {
            // std::cout << conn->value(0) << std::endl ;
            names.push_back(conn->value(0)) ; 
        }while(conn->next()) ; 
    } 
    return names ;
}