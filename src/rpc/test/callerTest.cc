#include "user.pb.h"
#include "./rpc/RpcChannel.h"

int main() 
{
    // 演示调用远程发布的 rpc 方法 Login 
    rpctest::UserServiceRpc_Stub stub(new RpcChannel());
    rpctest::LoginRequest request ;
    request.set_name("zhang san") ; 
    request.set_pwd("12346") ; 
    rpctest::LoginResponse response ;

    RpcController controller ; 
    stub.Login(&controller , &request , &response , nullptr) ; 

    // 发送 RPC 出错了
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl ; 
    }
    else 
    {
        // 一次 rpc 调用完成，读返回的参数结果
        if(0 == response.result().errcode())
        {
            std::cout << "rpc login response success : " << response.sucess() << std::endl ; 
        }
        else 
        {
            std::cout << "rpc login response error : " << response.result().errmsg() << std::endl ; 
        }
    }
    return 0 ; 
}