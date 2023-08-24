#include "./base/ZookeeperUtil.h"
#include <assert.h>
#include <iostream>

void testZookeeperCmd() 
{
    ZkClient zkClient_("/test") ;

    zkClient_.DeleteNode("/test") ;

    bool result = zkClient_.IsNodeExist("/test") ;
    assert(result == false) ; 
    
    // 测试创建永久 Service 节点
    result = zkClient_.CreateNode("/test" , nullptr , 0 , 0) ;
    assert(result == true) ; 

    std::cout << "create /test node" << std::endl ;

    // 测试创建临时节点 Service.Method 节点
    std::string tmpNode = "Login" ; 
    std::string tmpNodeValue = "127.0.0.1:8002" ; 
    result = zkClient_.CreateNode("/test/Login" , tmpNodeValue.data() , tmpNodeValue.size() , ZOO_EPHEMERAL) ; 
    assert(result == true) ; 
    
    std::string value = zkClient_.GetNodeData("/test/Login") ;
    std::cout << "/test/Login = " << value << std::endl ;
    assert(value == tmpNodeValue);

    zkClient_.SetNodeData("/test/Login" , "127.0.0.1:8003");
    value = zkClient_.GetNodeData("/test/Login") ;
    std::cout << "/test/Login = " << value << std::endl ;
    assert(value == "127.0.0.1:8003");

    // 测试创建临时顺序节点  
    result = zkClient_.CreateNode("/test/SequeLogin" , tmpNodeValue.data() , tmpNodeValue.size() , ZOO_EPHEMERAL | ZOO_SEQUENCE ) ; 
    assert(result == true) ; 
    sleep(20) ; 
}
int main() 
{
    testZookeeperCmd() ;
    
    return 0 ; 
}