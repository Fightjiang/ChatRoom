#include "./user.pb.h"
#include "./rpc/RpcProvider.h"


class UserService : public rpctest::UserServiceRpc 
{
public : 
    bool Login(std::string name , std::string pwd)
    {
        std::cout << "doing local service : Login " << std::endl ; 
        std::cout << "name : " << name << " pwd: " << pwd << std::endl ; 
        return false ; 
    }

    bool Register(uint32_t id , std::string name , std::string pwd) 
    {
        std::cout << "doing local service : Register " << std::endl ; 
        std::cout << "id" <<id << "name : " << name << " pwd: " << pwd << std::endl ; 
        return true ; 
    }

    /*
    重写基类 UserServiceRpc 的虚函数 , 下面这些方法都是 protoc 框架提供的，我们只需要重写对应的方法即可 , Protoc 所做的工作
    1. caller ===> Login(Login Request) => muduo => callee 
    2. callee ===> Login(LoginRequest)  => 转交下面重写的 Login 方法
    */

   void Login(::google::protobuf::RpcController* controller,
                       const ::rpctest::LoginRequest* request,
                       ::rpctest::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数 LoginRequest ， 应用获取相应数据做本地业务
        std::string name = request->name() ; 
        std::string pwd  = request->pwd()  ; 

        // 做本地业务 , 也就是接着调用上面的函数
        bool login_result = Login(name , pwd) ; 

        // 把响应写入 LoginResponse 
        rpctest::ResultCode *code = response->mutable_result() ; 
        code->set_errcode(0) ; 
        code->set_errmsg("") ; 
        response->set_sucess(login_result) ; 
        

        //std::cout<<response->result().errcode()<<" "<<response->result().errmsg()<<" "<<response->sucess()<<std::endl ;
        
        // 执行回调函数 , 执行响应对象数据的序列化和网络上发送（都是由我们写的 RPC 框架来完成）
        done->Run() ; 
    }

} ; 

int main(int argc , char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " IP Port" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = std::atoi(argv[2]);  // 将字符串转换为整数

    // provider 是一个 rpc 网络服务对象, 把 UserService 对象发布到 rpc 节点上
    RpcProvider provider ; 
    provider.NotifyService(new UserService()) ; 
    
    // 启动服务进行监听
    provider.Run(ip , port) ; 
    return 0 ; 
}