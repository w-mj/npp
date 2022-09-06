
# 网络模块

## 介绍

将TCP的流式连接包装成面向消息的通信方式。
NetworkManager的构造函数中必须提供一个int型的rank号作为标识，通信的各个节点之前rank不能重复。

通信前首先调用registerTarget()函数将对方的rank号与地址绑定。
随后即可调用sendMessage发送向该rank发送消息。

接收方在调用startNetworkThread()启动监听服务。随后可调用getMessage获得消息。
getMessage函数为阻塞式，即会等待收到一个完整的消息后才会返回。

## 报文格式

```
+------------------------+
|   Control | Reserve    |
|   4B Message Length    |
| ... Message Content ...|
|    4B Verify Code      |
+------------------------+
```

### Control 控制信息 
占用2字节。
```
15             8 7             0
+-------------------------------+
|R|R|R|R|R|R|R|R|R|R|R|R|R|R|V|C|
+-------------------------------+
```

+ C: 是否启用压缩，1为启用压缩，此时Message Length为压缩后的长度。
+ V: 是否启用校验，启用后Verify Code为Message Content的异或。若不启用校验则Verify Code为固定的0xDEAD_FACE。
+ R: 暂时保留。

### Reserve 保留位
占用2字节。

### Message Length 正文长度
占用4字节，无符号整数，表示接下来消息正文的长度。

### Message Content 消息正文
长度不定，由正文长度指定。

### Verify Code
占用4字节，为校验码。具体含义由控制信息指定。