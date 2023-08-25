#include "./user/UserMaster.h"

// 主节点也是负责转发消息给子节点的 , 所以这三个函数基本上没什么区别，除了需要加上 UserRequest 供子节点区分

void UserMaster::Login(::google::protobuf::RpcController *controller,
               const ::User::LoginRequest *request,
               ::User::LoginReponse *response,
               ::google::protobuf::Closure *done)
{
    // test 
    // std::cout << request->id() << " " << request->password() << " " << request->ip_port() << " " << std::endl ;

    //获取一个子节点信息
    int client_fd;
    while ((client_fd = zkClient_.GetFollowerFd()) == -1)
    {
        sleep(1);
    }
    User::UserRequest userRequest ;  
    userRequest.set_type("Login") ; 
    userRequest.set_message(request->SerializeAsString()) ; 

    std::string send_str = userRequest.SerializeAsString() ;

    //发送信息
    send(client_fd, send_str.c_str(), send_str.size(), 0);

    //获取信息
    char recv_buf[256] = {0};
    recv(client_fd, recv_buf, 256, 0);
    if (response->ParseFromString(recv_buf) == false)
    {
        std::cout << "userMaster Parse userFollower Login response Fail" << std::endl ;
    } 
    // std::cout << "result = " << response->is_success() << " msg = " << response->message() << std::endl ;
    close(client_fd);
    done->Run();
}

void UserMaster::Register(::google::protobuf::RpcController *controller,
                 const ::User::RegisterRequest *request,
                 ::User::RegisterResponse *response,
                 ::google::protobuf::Closure *done)
{
    //获取一个子节点信息
    int client_fd;
    while ((client_fd = zkClient_.GetFollowerFd()) == -1)
    {
        sleep(1);
    }
    User::UserRequest userRequest ;  
    userRequest.set_type("Register") ; 
    userRequest.set_message(request->SerializeAsString()) ; 

    std::string send_str = userRequest.SerializeAsString() ;

    //发送信息
    send(client_fd, send_str.c_str() , send_str.size(), 0);

    //获取信息
    char recv_buf[256] = {0};
    recv(client_fd, recv_buf, 256, 0);
    if (response->ParseFromString(recv_buf) == false)
    {
        std::cout << "userMaster Parse userFollower Register response Fail" << std::endl ;
    } 
    // std::cout << "result = " << response->is_success() << " msg = " << response->message() << std::endl ;
    close(client_fd);
    done->Run();
}

void UserMaster::LogOut(::google::protobuf::RpcController *controller,
                const ::User::LogOutRequest *request,
                ::User::LogOutResponse *response,
                ::google::protobuf::Closure *done)
{
    //获取一个子节点信息
    int client_fd;
    while ((client_fd = zkClient_.GetFollowerFd()) == -1)
    {
        sleep(1);
    }
    User::UserRequest userRequest ;  
    userRequest.set_type("LogOut") ; 
    userRequest.set_message(request->SerializeAsString()) ; 

    std::string send_str = userRequest.SerializeAsString() ;

    //发送信息
    send(client_fd, send_str.c_str() , send_str.size(), 0);

    //获取信息
    char recv_buf[256] = {0};
    recv(client_fd, recv_buf, 256, 0);
    if (response->ParseFromString(recv_buf) == false)
    {
        std::cout << "userMaster Parse userFollower Register LogOut Fail" << std::endl ;
    } 
    std::cout << "result = " << response->is_success() << " msg = " << response->message() << std::endl ;
    ::close(client_fd);
    done->Run();
}