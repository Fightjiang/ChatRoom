#ifndef USER_MASTER_H
#define USER_MASTER_H

#include "../user/User.pb.h"
#include "../base/ZookeeperUtil.h" 
#include "../rpc/RpcProvider.h"
#include <mysql/mysql.h>

class UserMaster : public User::UserServiceRpc
{
public:
    UserMaster(): zkClient_("/UserMaster") {}
    
    void Login(::google::protobuf::RpcController *controller,
               const ::User::LoginRequest *request,
               ::User::LoginReponse *response,
               ::google::protobuf::Closure *done) override ;

    void Register(::google::protobuf::RpcController *controller,
                 const ::User::RegisterRequest *request,
                 ::User::RegisterResponse *response,
                 ::google::protobuf::Closure *done) override;

    void LogOut(::google::protobuf::RpcController *controller,
                  const ::User::LogOutRequest *request,
                  ::User::LogOutResponse *response,
                  ::google::protobuf::Closure *done) override;

    void GetUserInfo(::google::protobuf::RpcController *controller,
                  const ::User::UserRequest *request,
                  ::User::GetUserInfoResponse *response,
                  ::google::protobuf::Closure *done) override;
                  
private:
    ZkClient zkClient_ ;  
} ; 


#endif