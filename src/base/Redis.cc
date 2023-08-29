#include "./base/Redis.h"
#include "./base/CommonConfig.h"
#include <iostream>

// 初始化redis连接信息
RedisCli::RedisCli()
{
    ip_   = ConfigInfo::redis_ip ;
    port_ = ConfigInfo::redis_port ;
    if(!connect()) 
    {
        std::cout << "Redis Server Connect Fail" << std::endl ;
    }
}

// 释放连接
RedisCli::~RedisCli()
{
    redisFree(get_channel_);
    redisFree(set_channel_);
}

// 初始化get_channel、set_channel连接
bool RedisCli::connect()
{
    get_channel_ = redisConnect(ip_, port_);
    if (get_channel_ == nullptr || get_channel_->err)
    {
        return false;
    }

    set_channel_ = redisConnect(ip_, port_);
    if (set_channel_ == nullptr || set_channel_->err)
    {
        return false;
    }

    return true;
}

// 判断某个键是否存在
bool RedisCli::key_exist(const std::string& key)
{
    if(this->get_key(key) == "")
    {
        return false ; 
    }
    return true ; 
}

// 获得 key 对应的键值
std::string RedisCli::get_key(const std::string& key)
{
    redisReply *reply = (redisReply *)redisCommand(get_channel_, "get %s", key.data());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Redis get key = " << key << " Error" << std::endl ;
        freeReplyObject(reply);
        return "";
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        std::cout << "Redis get key = " << key << " isn't exist" << std::endl ; 
        return "";
    }
    std::string result = reply->str;
    freeReplyObject(reply);
    return result ;
}

// 设置 key 对应的 value 值
bool RedisCli::set_key(const std::string& key, const std::string& host)
{
    redisReply *reply = (redisReply *)redisCommand(set_channel_, "set %s %s", key.data() , host.data());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Redis set key = " << key << " Error" << std::endl ;
        freeReplyObject(reply);
        return false ;
    }
    bool result = strcmp(reply->str, "OK") == 0;
    freeReplyObject(reply);
    return result ;
}

//删除 key 
bool RedisCli::del_key(const std::string& key)
{
    
    redisReply *reply = (redisReply *)redisCommand(set_channel_, "del %s" , key.data());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Redis del key = " << key << " Error" << std::endl ;
        freeReplyObject(reply);
        return false ;
    }
    // 没有这个 key 的话，那么返回的是 0 
    bool result = reply->integer >= 1 ;
    freeReplyObject(reply);
    return result ;
}


// 分布式锁 加锁
bool RedisCli::redisLock(const std::string& lock_key , const std::string& unique_value , const std::string& timeout) 
{
    char cmd[64] = {0} ; 
    sprintf(cmd , "SET %s %s NX PX %s" , lock_key.data() , unique_value.data() , timeout.data()) ; 
    redisReply *reply = (redisReply *)redisCommand(set_channel_, cmd);
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        std::cout << "Redis Lock set key = " << lock_key << " Error" << std::endl ;
        freeReplyObject(reply);
        return false;
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        return false;
    }
    bool result = strcmp(reply->str, "OK") == 0;
    freeReplyObject(reply);
    return result ;
} 

// 尝试解锁
bool RedisCli::redisUnLock(const std::string& lock_key , const std::string& unique_value) 
{

    std::string luaBash = "if redis.call('get',KEYS[1]) == ARGV[1] then return redis.call('del',KEYS[1]) else return 0 end";
    
    redisReply *reply = (redisReply *)redisCommand(get_channel_, "eval %s 1 %s %s" , luaBash.data() , lock_key.data() , unique_value.data() );
    
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR)
    {
        if(reply == nullptr) {
            std::cout << "reply Nullptr" << std::endl ;
        }
        std::cout << "Redis Release Lock Error" << std::endl ;
        freeReplyObject(reply);
        return false ;
    }
    
    // 没有这个 key 的话，那么返回的是 0 
    bool result = reply->integer >= 1 ;
    freeReplyObject(reply);
    return result ;
}
