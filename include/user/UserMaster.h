#ifndef USER_MASTER_H
#define USER_MASTER_H

#include "../user/UserService.pb.h"
#include "../base/ZookeeperUtil.h" 
#include "../rpc/RpcProvider.h"
#include <mysql/mysql.h>

class UserMaster  
{
public:
    UserMaster();
    
    void Login(::google::protobuf::RpcController *controller,
               const ::UserService::LoginRequest *request,
               ::UserService::LoginReponse *response,
               ::google::protobuf::Closure *done);

    void Registe(::google::protobuf::RpcController *controller,
                 const ::UserService::RegisterRequest *request,
                 ::UserService::RegisterResponse *response,
                 ::google::protobuf::Closure *done);

    void LogOut(::google::protobuf::RpcController *controller,
                  const ::UserService::LogOutRequest *request,
                  ::UserService::LogOutResponse *response,
                  ::google::protobuf::Closure *done);

private:
    ZkClient zkClient_ ;  
} ; 


#endif