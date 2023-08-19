#include "./base/Redis.h"
#include <iostream>
#include <assert.h>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

void testCommand() 
{
    RedisCli redisClient ; 
    bool result = redisClient.set_key("hello" , "world") ; 
    
    assert(result == true) ; 
    std::cout << "set hello world success" << std::endl ;
     
    std::string value = redisClient.get_key("hello") ;
    assert(value == "world") ;  
    std::cout << "get hello success" << std::endl ;
    
    result = redisClient.del_key("hello") ;
    assert(result == true) ; 
    std::cout << "already delete hello" << std::endl ;
    
    value = redisClient.get_key("hello"); 
    assert(value == "") ;
    std::cout << "cann't get hello " << std::endl ;
}

void testThread() 
{
    int num = 0 ; 
    auto func = [&]() {
        RedisCli redisClient ; 
        for(int i = 0 ; i < 50 ; ++i)
        {
            // 加上分布式锁
            while(redisClient.redisLock("testLock" , "lock") == false)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }

            // 执行任务
            std::cout << "thread id = " << std::this_thread::get_id() << " num = " << ++num << std::endl ;
            
            // 解锁
            if(redisClient.redisUnLock("testLock" , "lock") == false)
            {
                std::cout << "UnLock Fail" << std::endl ;
            }
        }
    } ;

    std::vector<std::thread> threadVec ; 
    for(int i = 0 ; i < 10 ; ++i) 
    {
        threadVec.push_back(std::thread(func)) ; 
    }

    for(int i = 0 ; i < 10 ; ++i) 
    {
        threadVec[i].join() ; 
    }

    std::cout<< "num = " << num << std::endl ;
    assert(num == 500) ;
}

int main()
{
    // testCommand() ;
    testThread() ;
    // RedisCli redisClient ; 
    // while(redisClient.redisLock("testLock" , "lock") == false)
    // {
    //     std::this_thread::sleep_for(std::chrono::microseconds(200));
    // }

    // if(redisClient.redisUnLock("testLock" , "lock") == false)
    // {
    //     std::cout << "UnLock Fail" << std::endl ;
    // }
    return 0 ; 
}