## Nginx 配置

### 1. Ubuntu 按照 Nginx
Nginx 在默认的 Ubuntu 源仓库中可用。想要安装它，运行下面的命令即可：
```
sudo apt update
sudo apt install nginx
```
Nginx 启动/查看/关闭相关命令：
```
sudo service nginx start/status/stop
```

测试是否安装成功，访问你的主机`http://localhost` ,出现下图则表示安装成功了
![](https://imgconvert.csdnimg.cn/aHR0cHM6Ly91cGxvYWQtaW1hZ2VzLmppYW5zaHUuaW8vdXBsb2FkX2ltYWdlcy80MTQxNjYtNWY5MDkwZTE2ZGI5YTM1OC5wbmc_aW1hZ2VNb2dyMi9hdXRvLW9yaWVudC9zdHJpcHxpbWFnZVZpZXcyLzIvZm9ybWF0L3dlYnA?x-oss-process=image/format,png)

### 2. Nginx 配置文件结构
* 所有的 Nginx 配置文件都在/etc/nginx/目录下。
* 主要的 Nginx 配置文件是/etc/nginx/nginx.conf，自己定义的配置文件可以放在 /etc/nginx/conf.d 中
* Nginx 日志文件(access.log 和 error.log)定位在/var/log/nginx/目录下

### 3. 配置 TCP 负载均衡
Nginx 的 TCP/UDP 负载均衡是应用 Stream 代理模块（ngx_stream_proxy_module）和 Stream 上游模块（ngx_stream_upstream_module）实现的,具体的代码配置如下所示
```
# /etc/nginx/conf.d/nginx.config
# nginx tcp loadbanlance config
stream {
    upstream ProxyServer {
        # 如果需要添加新的机器就继续往配置里面加上服务器信息即可
        # 里面的两行是服务器的信息，weight是权重，值越高负载均衡时分配给其的客户端连接就越多，1:1则是轮询
        #  max_fails是"心跳数"为三，一般来说，一旦Nginx和服务器之间连接三次均超时，则放弃连接
        # 在 fail_timout = 30s 的时间内出现 3 次不可用情况，则判定节点不可用，等到 30s 之后再重新检测节点健康情况

        server 127.0.0.1:8001 weight=1 max_fails=3 fail_timeout=30s;        
        server 127.0.0.1:8002 weight=1 max_fails=3 fail_timeout=30s;
    }   
    server {
        # 连接 proxy_Server 服务器的超时时间
        proxy_connect_timeout 5s;
        # listen 8000，是客户端要连接的端口号
        listen 8000;
        # 数据流走就是 ProxyServer 上面的配置
        proxy_pass ProxyServer;
        # 关闭 Nagle 算法
        tcp_nodelay on;
    }
}
```
特别注意，由于 Nginx 规定了，`http {}` 和 `stream {}` 块之间是水平独立的，故我们还需要修改 `/etc/nginx/nginx.conf` 中的配置文件，将 `include /etc/nginx/conf.d/*.conf;` 这行代码移出 `http{}` , 否则的话，会报 `nginx: [emerg] "stream" directive is not allowed here in /etc/nginx/conf.d/load-balancer.conf:3` 错误

至此，该项目客户端的流量负载均衡转发到 ProxyServer 上就配置完成了，选择 Nginx 作为集群的负载均衡模块的原因有：
1. 能把 Client 的请求按照负载均衡算法分发到具体的 ProxyServer 服务器上
2. 能够与 ProxyServer 保持心跳连接机制，检查 ProxyServer 故障
3. 能够发现新添加的 ProxyServer 机器，方便扩展服务器的数量

  
参考文献：
1. [Nginx stream 配置代理（Nginx TCP/UDP 负载均衡）](https://blog.51cto.com/u_1472521/4965232)
2. [如何在 Ubuntu 20.04 上安装 Nginx](https://zhuanlan.zhihu.com/p/138007915)
3. [Ubuntu 安装Nginx及简单配置](https://blog.csdn.net/shenmingxueIT/article/details/113186948)