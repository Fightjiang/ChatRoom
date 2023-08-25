#include "./user/UserMaster.h"

UserMaster::UserMaster() : zkClient_("/UserMaster") {}

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
        std::cout << "userMaster Parse userFollower response Fail" << std::endl ;
    } 
    std::cout << "result = " << response->is_success() << " msg = " << response->message() << std::endl ;
    close(client_fd);
    done->Run();
}

void UserMaster::Register(::google::protobuf::RpcController *controller,
                 const ::User::RegisterRequest *request,
                 ::User::RegisterResponse *response,
                 ::google::protobuf::Closure *done)
{

}

void UserMaster::LogOut(::google::protobuf::RpcController *controller,
                const ::User::LogOutRequest *request,
                ::User::LogOutResponse *response,
                ::google::protobuf::Closure *done)
{
    std::cout << "LOGOUT" << std::endl ;
    done->Run();
}