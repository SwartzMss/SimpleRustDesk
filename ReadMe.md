# SimpleRustDesk

[![CI Build](https://github.com/SwartzMss/SimpleRustDesk/actions/workflows/msbuild.yml/badge.svg)](https://github.com/SwartzMss/SimpleRustDesk/actions/workflows/msbuild.yml)

SimpleRustDesk 是一个远程桌面控制系统示例项目，参考 RustDesk 的设计思想，展示了如何构建一个完整的远程控制框架。系统通过信令交互和数据中继，实现被控端与控制端之间的远程连接与数据传输，主要包含以下模块：

## 框架概述

- **IDServer（信令服务器）**  
  信令服务器作为系统的中枢，负责管理被控端和控制端之间的信令交互。所有被控端（DeskServer）和控制端（DeskControler）均通过 IDServer 进行注册与状态交换，从而确定各终端的在线状态。

- **DeskServer（被控端）**  
  被控端启动后会向 IDServer 注册自己的在线信息，供控制端查询。与此同时，DeskServer 启动中继模块，通过与 RelayServer 的连接接收并响应来自控制端的控制数据，实现远程控制。

- **DeskControler（控制端）**  
  控制端通过 IDServer 查询目标被控端的在线状态，并发起远程控制请求。实际控制数据传输由中继服务器（RelayServer）进行中继转发，确保数据传输的实时性和可靠性。

- **RelayServer（中继服务器）**  
  在信令交互完成后，所有被控端与控制端之间的实时数据传输均由 RelayServer 中继。RelayServer 负责将 DeskControler 的控制指令转发至 DeskServer，并将被控端的反馈数据返回给控制端，从而实现数据的双向传输。

## 注意事项

- 本项目为示例性质，主要展示远程控制系统的整体架构和基本实现。
- 实际应用中可能需要扩展屏幕共享、远程输入、数据加密等功能，以满足更高的安全性和性能要求。

## 贡献

欢迎各位开发者通过提交 PR 或 issue 来改进和扩展本项目。如有疑问或建议，请在 GitHub 上联系项目维护者。

## 许可

本项目采用 [MIT 许可](LICENSE)。
