# Websocket

## 项目简介

在树莓派上基于 libevent 库编程实现高并发连接的 Websocket 服务器程序，实时响应Websocket客户端的数据/控制请求；
客户端使用 Bootstrap 前端框架编写 HTML 控制界面，提供人机交互界面，实现温湿度的实时显示和LED灯的远程控制。

websocket解析相关的源码 https://github.com/mortzdk/websocket



## 项目功能

1. **高并发 WebSocket 服务器**：
   - 服务器能够处理多个客户端的连接请求，支持高并发的数据交换。
   - 实现 WebSocket 协议的握手、数据传输和帧处理。
   - 实时处理来自客户端的控制请求。
   - 向客户端下发数据。
2. **温湿度实时显示**：
   - 服务器从SHT20温湿度传感器获取实时的温湿度数据。
   - 通过 WebSocket 将温湿度数据传输到客户端，客户端页面实时更新显示。
3. **LED 灯远程控制**：
   - 客户端界面提供控制 LED 灯的功能。
   - 用户通过界面操作，发送控制指令到 WebSocket 服务器。
   - 服务器接收指令并通过 GPIO 控制 LED 灯的状态。
4. **前端界面**：
   - 使用 Bootstrap 框架构建的 HTML 页面。
   - 提供用户友好的界面来显示温湿度数据和控制 LED 灯。
   - 支持与 WebSocket 服务器进行双向通信。



## 目录结构

```
.
├── include
│   ├── b64.h			#base64编码
│   ├── frame.h			#解析和创建帧，根据源码改写
│   ├── index_html.h	#前端页面，建立连接后由服务器返回给客户端
│   ├── led.h			#解析客户端的LED控制请求，基于libgpiod控制LED灯
│   ├── logger.h		#日志系统
│   ├── mutex.h			#锁，防止并发访问
│   ├── predict.h		#定义了两个宏likely和unlikely，用于向编译器提示某个条件在大多数情况下是否为真
│   ├── sha1.h			#SHA1算法
│   ├── sht20.h			#温湿度采样
│   └── wss.h			#处理websocket请求
├── makefile
├── openlibs			#开源库，使用shell脚本下载安装，makefile管理
│   ├── cJSON
│   │   ├── build.sh
│   │   └── makefile
│   └── libevent
│       ├── build.sh
│       └── makefile
├── README.md
├── src
│   ├── b64.c
│   ├── frame.c
│   ├── index.c
│   ├── led.c
│   ├── logger.c
│   ├── makefile
│   ├── mutex.c
│   ├── sha1.c
│   ├── sht20.c
│   └── wss.c
├── tools				#前端页面，使用shell脚本生成.c文件
│   ├── convert.sh
│   └── index.html
└── websocket.c			
```





## 使用

下载安装开源库

```sh
cd Websocket/openlibs/cJSON
make
cd Websocket/openlibs/libevent
make
```

注：本项目还使用到了libgpiod，需要自行下载安装



如果修改了tools下的index.html

```sh
./convert.sh index.html
mv index.c ../src
```



编译

```sh
cd Websocket/src
make
```



运行

```sh
make
make run
```



## 技术栈

- **服务器端**：
  - 编程语言：C
  - 库：libevent、libgpiod、cJSON
  - 操作系统：Linux
- **客户端**：
  - 技术：HTML、CSS、JavaScript
  - 框架：Bootstrap
  - WebSocket 协议
- **硬件**：
  - 传感器：SHT20
  - 控制器：树莓派
  - LED 灯：用于远程控制



