## 项目简介
实现一个基于 libevent 和 OpenSSL 库的客户端与服务器端加密通信系统。



### 项目目标

1. **实现加密通信**：通过在客户端和服务器端之间建立安全的加密通道，确保数据在传输过程中的机密性和完整性。
2. **利用 libevent 进行高并发处理**：使用 libevent 库来处理高并发连接和事件驱动的 I/O 操作，提升系统的性能和响应能力。
3. **使用 OpenSSL 提供加密服务**：集成 OpenSSL 库，利用其强大的加密算法和证书管理功能，实现数据的加密和解密操作。
4. **确保数据安全性**：通过 SSL/TLS 协议保护通信过程中的数据，防止数据被窃取或篡改。



### 技术栈

- **libevent**：一个事件驱动的网络库，支持高效的异步 I/O 操作和事件通知，适用于高并发环境。
- **OpenSSL**：一个广泛使用的加密库，提供各种加密算法和协议（如 SSL/TLS），用于保护数据的机密性和完整性。





## 目录结构

```makefile
.
├── client
│   ├── include
│   │   ├── database.h       # 数据库操作相关的头文件
│   │   ├── ds18b20.h        # DS18B20 传感器相关的头文件
│   │   ├── logger.h         # 日志记录相关的头文件
│   │   ├── packet.h         # 数据包处理相关的头文件
│   │   └── socket_event.h   # 套接字事件处理相关的头文件
│   ├── main.c               # 客户端程序入口
│   ├── makefile             # 客户端部分的构建脚本
│   ├── openlibs
│   │   ├── cJSON
│   │   │   ├── build.sh     # cJSON 库的构建脚本
│   │   │   └── makefile     
│   │   ├── libevent
│   │   │   ├── build.sh     
│   │   │   └── makefile    
│   │   ├── openssl
│   │   │   ├── build.sh    
│   │   │   └── makefile     
│   │   └── sqlite
│   │       ├── build.sh     
│   │       └── makefile     
│   └── src
│       ├── database.c       # 数据库操作相关的源文件
│       ├── ds18b20.c        # DS18B20 传感器相关的源文件
│       ├── logger.c         # 日志记录相关的源文件
│       ├── makefile         # 客户端源文件的构建脚本
│       ├── packet.c         # 数据包处理相关的源文件
│       └── socket_event.c   # 套接字事件处理相关的源文件
├── README.md                # 项目说明文件
└── server
    ├── include
    │   ├── database.h       
    │   ├── logger.h         
    │   └── socket_event.h   
    ├── main.c               
    ├── makefile             
    ├── openlibs
    │   ├── libevent
    │   │   ├── build.sh    
    │   │   └── makefile    
    │   ├── openssl
    │   │   ├── build.sh     
    │   │   └── makefile     
    │   └── sqlite
    │       ├── build.sh     
    │       └── makefile    
    ├── src
    │   ├── database.c      
    │   ├── logger.c        
    │   ├── makefile        
    │   └── socket_event.c   
    └── ssl
        ├── cacert.pem       # 证书颁发机构证书
        └── privkey.pem      # 服务器私钥
```





## 使用说明

### 生成密钥和证书

```sh
openssl genrsa -out privkey.pem 2048#生成服务器私钥

openssl req -new -x509 -key privkey.pem -out cacert.pem -days 3650   #生成自签名证书
```





