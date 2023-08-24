#ifndef USER_MASTER_H
#define USER_MASTER_H

#include "../user/User.pb.h"
#include "../base/ZookeeperUtil.h" 
#include "../rpc/RpcProvider.h"
#include <mysql/mysql.h>

class UserMaster : public User::UserServiceRpc
{
public:
    UserMaster();
    
    void Login(::google::protobuf::RpcController *controller,
               const ::User::LoginRequest *request,
               ::User::LoginReponse *response,
               ::google::protobuf::Closure *done);

    void Register(::google::protobuf::RpcController *controller,
                 const ::User::RegisterRequest *request,
                 ::User::RegisterResponse *response,
                 ::google::protobuf::Closure *done);

    void LogOut(::google::protobuf::RpcController *controller,
                  const ::User::LogOutRequest *request,
                  ::User::LogOutResponse *response,
                  ::google::protobuf::Closure *done);

private:
    ZkClient zkClient_ ;  
} ; 


#endif