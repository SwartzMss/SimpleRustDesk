# SimpleRustDesk

[![CI Build](https://github.com/SwartzMss/SimpleRustDesk/actions/workflows/msbuild.yml/badge.svg)](https://github.com/SwartzMss/SimpleRustDesk/actions/workflows/msbuild.yml)

SimpleRustDesk 是一个远程桌面控制系统示例项目，参考 RustDesk 的设计思想，展示了如何构建一个完整的远程控制框架。系统通过信令交互和数据中继，实现被控端与控制端之间的远程连接与数据传输，主要包含以下模块：

## 框架概述

- **IDServer（信令服务器）**  
  信令服务器作为系统的中枢，主要负责管理被控端的注册与状态交换。控制端无需注册，只需手动输入目标设备 ID 发起连接请求。

- **DeskServer（被控端）**  
  被控端启动后会向 IDServer 注册自己的在线信息，供远程控制使用。同时，DeskServer 会检测 RelayServer 的在线状态，以确保中继服务可用。被控端支持分布式部署，无需自行启动中继模块。

- **DeskControler（控制端）**  
  控制端通过手动输入目标被控端的 ID 发起连接请求。一旦确认连接，控制端依赖 RelayServer 进行数据中继，从而实现远程控制操作。

- **RelayServer（中继服务器）**  
  RelayServer 提供数据转发服务，确保远程控制过程中的数据能够顺畅传输。

```mermaid
flowchart TD
    A[DeskServer (DS) 向 IDServer (IDS) 注册在线信息]
    B[DS 检测 RelayServer (RS) 状态]
    C[DeskControler (DC) 输入目标 DS 的 ID 并发送请求给 DS]
    D{目标 DS 是否存在且在线?}
    E[DS 直接返回失败给 DC]
    F[IDS 向 DS 发送消息]
    G[DS 检查 RS 状态，获取 RS 的 IP 和端口信息]
    H[DS 将 RS 信息反馈给 IDS]
    I[IDS 将 RS 信息转发给 DC]
    J[DC 与 DS 分别与 RS 建立连接]
    K[RS 开始进行数据转发]

    A --> B
    B --> C
    C --> D
    D -- 否 --> E
    D -- 是 --> F
    F --> G
    G --> H
    H --> I
    I --> J
    J --> K
复制


## 注意事项

- 本项目为示例性质，主要展示远程控制系统的整体架构和基本实现。
- 实际应用中可能需要扩展屏幕共享、远程输入、数据加密等功能，以满足更高的安全性和性能要求。

## 贡献

欢迎各位开发者通过提交 PR 或 issue 来改进和扩展本项目。如有疑问或建议，请在 GitHub 上联系项目维护者。

## 许可

本项目采用 [MIT 许可](LICENSE)。
