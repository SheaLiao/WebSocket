# Websocket

## 项目简介

在STM32单片机上使用ESP8266WI-FI模块联网实现WebSocket服务器，实时响应Websocket客户端的数据/控制请求，使用串口AT发送和接收数据； 客户端使用 Bootstrap 前端框架编写 HTML 控制界面，提供人机交互界面，实现温湿度的实时显示和LED灯的远程控制。

websocket解析相关的源码 https://github.com/mortzdk/websocket





## 目录结构

```
.
├── Core
│   ├── Inc
│   │   ├── b64.h					#base64编码
│   │   ├── esp8266.h				#esp8266联网，设置服务器
│   │   ├── frame.h					#解析、创建websocket帧
│   │   ├── gpio.h					#gpio控制LED灯
│   │   ├── gpio_i2c_sht20.h		#gpio模拟I2C进行温湿度采样
│   │   ├── index_html.h			#html页面
│   │   ├── main.h
│   │   ├── ringbuf.h				#循环缓冲区，防止接收数据时接收不完整或数据覆盖
│   │   ├── sha1.h					
│   │   ├── sht20.h					#使用HAL进行温湿度采样
│   │   ├── tim.h					#延时
│   │   ├── usart.h					#串口接收和发送
│   │   └── wss.h					#处理websocket请求
│   └── Src
│       ├── b64.c
│       ├── esp8266.c
│       ├── frame.c
│       ├── gpio.c
│       ├── gpio_i2c_sht20.c
│       ├── index.c
│       ├── main.c
│       ├── ringbuf.c
│       ├── sha1.c
│       ├── sht20.c
│       ├── tim.c
│       ├── usart.c
│       └── wss.c
└── README.md
```



