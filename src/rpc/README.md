## RPC 远程调用详解
1. RPC 框架的作用时将 `本机调用远程方法 发送 序列化数据 到目标主机执行方法 并接收返回的序列化数据` 的过程都封装起来。 客户端需要发送序列化请求数据；服务端接收序列化请求数据->反序列化->执行本地方法->序列化响应数据->发送响应序列化数据给客户端。

2. 我们这里是使用 muduo 网络库提供底层网络字符串序列传输；通过 Zookeeper 实现 RPC 数据统一配置管理。

3. RpcChannel 类主要是对应重写 Probobuf 中 Channel 的的 CallMethod 方法，则在客户端所有的RPC方法调用最终都会统一到 channel_->CallMethod(descriptor()->method(idx),controller,request,response,done) 进行处理。，来实现在客户端发送对应约定好的字符串序列格式 (headerSize + service_name + method_name + argc_size + argc_str) ; 其中 headerSize 固定为 4 个字节，表示 service_name + method_name + argc_size 的长度，其中的 (service_name + method_name + argc_size) 是可以被 protobuf 序列化解析提取出来的。 然后再通过 argc_size 可以进一步解析出 args_str 所带的请求参数。 故我们实现在客户端所有的调用方法都能被我们框架中的 CallMethod 中统一处理。
4. RpcProvider 模块主要是对应 RpcServer 来监听客户端请求并进行处理。
    * 用户可以通过调用 notify_service() 在该 RPC 服务器上注册对应的方法(类+方法)，并通过 Map 数组保存下来，便于客户端序列化数据过来的时候，服务器可以马上判断是否有提供该对应的远程调用方法。
    * Run() 函数是通过在 Zookeeper 上发布对应的 Service/Method 所在的服务器节点，实际上就是 RpcServer 服务的IP地址（IP:Port），并启动 muduo 网络监听客户端请求。
    * on_Message() 解析客户端发送过来的字符序列，然后调用对应的 Service->CallMethod() 方法函数处理请求，再将 response 返回给客户端。注意 Service->CallMethod() 是统一通过 Protobuf 框架在底层调用生成代码中的 method->index() 对应执行相应的方法。


假设客户端调用的 Login 方法，那么整个数据流的走向是：
* 客户端： stub.Login()->channel_.CallMethod()->RpcChannel.CallMethod()（这个就是我们上面自己写的客户端序列化发送请求）【欧克，客户端就在这里发送完了，等待服务端接收请求并响应发送给客户端】
* 服务端： server.run()->server.on_Message()【解析处理客户端发送过来的数据】-> service->CallMethod()【调用用户在服务端上注册的请求】-> server.send_rpc_response()【发送 response 给客户端】

其中 Protobuf 框架不仅提供了 request 和 response 的序列化和反序列化，还提供了协调方法统一调用的：Channel、Service、MethodDescriptor等类。


我们项目中的 ProxyServer 启动代理服务转发功能，再调用对应的远程服务机器上的方法，这个过程就是 RPC 远程调用框架的知识，将 protobuf 序列化的数据发送给远程服务器，远程服务器解序列化之后再调用函数获得对应的返回值，将返回值序列化返回回去。
