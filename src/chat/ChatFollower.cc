#include "./chat/ChatFollower.h"

void ChatFollower::run(const char * ip , const size_t port)
{
    //初始化网络层
    muduo::net::InetAddress address(ip , port);
    muduo::net::TcpServer server(&loop_, address, "ChatFollower");

    server.setMessageCallback(std::bind(&ChatFollower::on_message, this, 
                               std::placeholders::_1, 
                               std::placeholders::_2, 
                               std::placeholders::_3));

    server.setConnectionCallback(std::bind(&ChatFollower::on_connetion, this, 
                                 std::placeholders::_1));


    // 把自己的节点信息注册到 zookeeper 上，供主节点查找和发现 , 临时顺序节点
    std::string server_path = "/ChatMaster/server";
    char nodeValue[64] = {0};
    sprintf(nodeValue, "%s:%lu", ip , port);
    if (zkClient_.CreateNode(server_path.data(), nodeValue, strlen(nodeValue), ZOO_EPHEMERAL | ZOO_SEQUENCE) == false)
    {
        std::cout << "zookeeper create ZOO_SEQUENCE node fail path = " << server_path << " data = " << nodeValue << std::endl ;
        return ; 
    } 
    //开启事件循环
    std::cout << "ChatFollower ip: " << ip <<" port: " << port <<" run " << std::endl ;
    server.setThreadNum(4);
    server.start();
    loop_.loop();
}

// 可读事件到来之后的回调函数
void ChatFollower::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp stamp)
{
    //反序列化由 ProxyService 传过来的序列化数据
    std::string recvStr = buffer->retrieveAllAsString();
    ChatMessage::ChatRequest chatRequest;
    chatRequest.ParseFromString(recvStr) ;
    std::string methodType = chatRequest.type() ; 
    std::string responseMessage ; 
    if (methodType == "ChatMessage")
    {
        sendChatMessage(chatRequest.message()) ;
    }
    else if (methodType == "WriteOffline") // 由于 proxyServer 发送消息给用户失败，也写入离线消息
    {
        ChatMessage::Message chatMessage ; 
        chatMessage.ParseFromString(recvStr) ;
        writeOffline(chatMessage.sendname() , chatMessage.recvname() , chatMessage.msg()) ;
    }
    else if (methodType == "ReadOffline")
    {
        ChatMessage::ReadOfflineRequest readRequest ; 
        readRequest.ParseFromString(recvStr) ;
        ChatMessage::ReadOfflineResponse readOfflineResponse; 

        readOffline(readRequest.name() , &readOfflineResponse) ;
        responseMessage = readOfflineResponse.SerializeAsString() ; 
    }
    
}

//连接事件回调函数
void ChatFollower::on_connetion(const muduo::net::TcpConnectionPtr &conn)
{
    //关闭连接
    if (!conn->connected())
    {
        conn->shutdown();
    }
}

// 智能指针管理 Socket ，可以放心使用 client_fd 不用担心忘记 ::close() 的情况了
std::shared_ptr<Socket> ChatFollower::establish_connection(const std::string& host)
{
    //解析ip和port
    int index = host.find(":");
    std::string ip = host.substr(0, index);
    int port = ::atoi(host.substr(index + 1, host.size() - index).data());

    //创建socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        std::cout << "create client fd error" << std::endl ;
        return nullptr ;
    }

    //配置服务器信息
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //连接
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cout << "establish connect error" << std::endl ;
        close(client_fd);
        return nullptr ;
    }
    std::shared_ptr<Socket> socketFd(new Socket(client_fd)) ; 
    return socketFd;
}

std::string ChatFollower::queryNameId(const std::string &name)
{
    char sql[64] = {0};
    sprintf(sql, "select id from user where name='%s'", name.data()); 
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
    if (conn->query(sql) && conn->next())  
    {
        return conn->value(0).data() ; 
    } 
    return "" ; // name 不存在
}

bool ChatFollower::writeOffline(const std::string& sendName , const std::string& recvName , 
                                const std::string& message , const int flag)
{
    std::string sendID = queryNameId(sendName) ; 
    std::string recvID = queryNameId(recvName) ;
    if(sendID.size() < 0 || recvID.size() < 0) 
    {
        std::cout << "sendName = " << sendName << " recvName = " << recvName <<" is not exist"<< std::endl ;
        return false ;
    }
    // 异常情况 proxyServer 转发给用户端消息失败，重新更改离线消息的状态为 UNREAD
    if(flag == -1) {
        char sql[256] = {0};
        sprintf(sql , "select id from offlinemessage \
        where recvid = '%s' and sendid = '%s' and message = '%s' and status = 'READ'" , recvName.data() , sendName.data() , message.data()) ;
        std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
        if (conn->query(sql) && conn->next())  
        {
            std::string msgId = conn->value(0).data() ; 
            memset(sql , '\0' , sizeof(sql)) ; 
            sprintf(sql, "UPDATE `offlinemessage` set status = 'READ' \
                  where id in (%s)" , msgId.data()); 
            std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
            return conn->update(sql) ;
        }  
        std::cout << "this message is not exist" << "sendName = " << sendName
                    << "recvName = " << recvName << " message = " << message << std::endl ;
        return false ; 
    }
    char sql[256] = {0};
    sprintf(sql, "insert into `offlinemessage`(recvid , sendid , message) values('%s','%s','%s')", 
                    recvID.data() , sendID.data() , message.data()); 
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
    return conn->update(sql);
} 

std::vector<std::string> ChatFollower::readOffline(const std::string& Name , 
                     ChatMessage::ReadOfflineResponse *response)
{
    std::string nameID = queryNameId(Name) ; 
    if(nameID.size() < 0) 
    {
        std::cout <<" recvName = " << Name <<" is not exist"<< std::endl ;
        return {} ;
    }

    char sql[258] = {0};
    sprintf(sql, "select a.id , b.name , a.message , a.datetime from offlinemessage a left join user b on a.sendid = b.id \
                  where a.recvid =%s" , nameID.data()); 

    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
    std::vector<std::string> messageIDs ; 
    if (conn->query(sql) && conn->next())  
    {
        do {
            std::cout << conn->value(1) << " " << conn->value(2) << " " 
                      << conn->value(3) << std::endl ;

            ChatMessage::Message message ; 
            message.set_recvname(Name) ; message.set_sendname(conn->value(1)) ; 
            message.set_msg(conn->value(2)) ; message.set_time(conn->value(3)) ; 
            response->add_messages()->CopyFrom(message) ; 
            messageIDs.push_back(conn->value(0)) ; 
        }while(conn->next()) ; 
    } 
    return messageIDs ;
}

bool ChatFollower::deleteOffline(const std::vector<std::string> & messageIDs) 
{
    std::string messageIdStr ;
    for(int i = 0 ; i < messageIDs.size() ; ++i) {
        messageIdStr += messageIDs[i] ;
        if(i + 1 != messageIDs.size()) {
            messageIdStr.push_back(',') ;
        }
    }
    char sql[256] = {0};
    sprintf(sql, "UPDATE `offlinemessage` set status = 'READ' \
                  where id in (%s)" , messageIdStr.data()); 
    std::shared_ptr<MysqlConn> conn = MysqlPool_->getConnection();
    return conn->update(sql);
}

void ChatFollower::sendChatMessage(const std::string& recvStr)
{
    ChatMessage::Message chatMessage ; 
    chatMessage.ParseFromString(recvStr) ; 
    std::string host = redisClient_.get_key(chatMessage.recvname()) ;
    //如果当前用户没登陆,向 Mysql 写入离线消息
    if (host == "")
    {
        if( writeOffline(chatMessage.sendname() , chatMessage.recvname() , chatMessage.msg()) == false) 
        {
            std::cout << "write offline message fail" << std::endl ;
        }
        return ;
    }
    // 多个 subReactor 会返回服 ChatFollower 的公有 chanel_map 资源，故需要加锁
    std::shared_ptr<Socket> cliend_fd = nullptr ; 
    {
        std::unique_lock<std::mutex> locker(mutex_) ; 
        auto it = channel_map_.find(host);
        if (it == channel_map_.end()) //记录里面没有这个链接
        {
            locker.unlock() ; 
                
                cliend_fd = establish_connection(host);
                if(cliend_fd == nullptr) {
                    std::cout << "ChatFollower establish_connection() Fail" << std::endl ;
                }

            locker.lock() ;
                channel_map_[host] = cliend_fd;
        } else {
            cliend_fd = it->second ;
        }
    }

    // 发送消息给用户 host 所在的 Proxy 服务器，请求帮忙转发
    Proxy::ProxyRequest proxyRequest ; 
    proxyRequest.set_type("forwardMessage") ; 
    proxyRequest.set_request_msg(recvStr) ;

    std::string send_str = proxyRequest.SerializeAsString();
    if (send(cliend_fd->fd() , send_str.c_str(), send_str.size(), 0) == -1) 
    {
        std::cout << "ChatFollower send error" << std::endl ;
    }
}