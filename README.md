# AI 开发系统 (AI Dev System)

> **模拟一家 AI 软件公司** — 你输入需求，AI 项目经理拆解任务，AI 开发员工直接在你电脑上写代码、编译、运行。

[![.NET](https://img.shields.io/badge/.NET-8.0-512BD4)](https://dotnet.microsoft.com/)
[![WPF](https://img.shields.io/badge/WPF-Windows%20Desktop-7C3AED)](https://learn.microsoft.com/dotnet/desktop/wpf/)
[![DeepSeek](https://img.shields.io/badge/AI-DeepSeek%20API-00a86b)](https://platform.deepseek.com/)

## 这是什么？

一个 Windows 桌面应用，用 WPF (.NET 8.0) 构建。把 AI 协作开发做成"公司"模型：

| 角色 | AI 模型 | 做什么 |
|------|---------|--------|
| **项目经理 (PM)** | DeepSeek | 讨论需求、拆解子任务、审查代码质量 |
| **开发员工 (Dev)** | DeepSeek | 写代码、调编译器、修复错误、打包交付 |

## 快速开始

### 前置要求
- Windows 10/11
- [.NET 8.0 SDK](https://dotnet.microsoft.com/zh-cn/download/dotnet/8.0)
- [DeepSeek API Key](https://platform.deepseek.com/)

### 运行
```bash
cd AIProjects/AIDevSystem
dotnet run --project src/AIDevSystem
```

### 使用
1. **设置页**填 API Key
2. 新建项目 → 输入需求 → **拆解** → **执行**
3. 或先 **与PM讨论** 聊清楚需求再拆解

## 核心特性

- **双 AI 协作** — PM 拆任务 + 审查，Dev 写代码 + 编译
- **完整 Shell 权限** — AI 能调 g++、cargo、dotnet、任何命令
- **自动编译验证** — 写完代码自动编译，失败自动修复
- **PM 审查循环** — 代码不通过自动打回重做（最多 3 次）
- **进度跟踪** — 实时任务完成百分比
- **聊天讨论** — 和 AI PM 对话确认需求
- **评分记忆** — 每次工作后评分，越用越准
- **内置帮助** — 用户指南内嵌在应用中

## 技术栈

| 层 | 技术 |
|----|------|
| UI | WPF (.NET 8.0) + MVVM + 暗色主题 |
| 数据库 | SQLite |
| AI | DeepSeek API (HttpClient 直连) |
| JSON | Newtonsoft.Json |

## 项目结构
```
AIProjects/
├── AIDevSystem/            # WPF 主项目
│   └── src/AIDevSystem/
│       ├── Models/          # 数据模型
│       ├── ViewModels/      # MVVM 视图模型
│       ├── Views/           # XAML 视图
│       ├── Services/        # 业务逻辑
│       ├── Infrastructure/  # API 客户端 + 适配器
│       ├── Themes/          # 暗色主题
│       └── Controls/        # 自定义控件
└── agent_memory/            # AI 员工记忆文件夹
```

## 关于

**作者**: 刘家瑞 (LjiaruiAT) — 河北科技大学  
**开发方式**: 完全 AI 辅助 — Claude Code (PM) + ReasonX (Dev)  
**开发日志**: `桌面\开发记录\`

> 一个学生在 2 天里，用自己搭建的 AI 系统，做出的 AI 开发工具。
> AI 没有替代学习，它加速了学习。
