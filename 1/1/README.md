# 需求解析模块 (Requirement Parsing Module)

## 概述

将用户的自然语言需求进行结构化解析，提取关键实体、意图和约束条件，生成标准化的需求描述JSON。

## 模块架构

```
RequirementParser/
├── RequirementParser.hpp    # 头文件：数据结构 + 解析器接口
├── RequirementParser.cpp    # 实现：解析引擎核心逻辑
├── main.cpp                 # 演示程序 + 交互模式
├── CMakeLists.txt           # CMake 构建配置
└── build.bat                # Windows 一键编译脚本
```

## 核心能力

| 能力 | 说明 |
|------|------|
| **实体提取** | 识别用户角色(actor)、操作目标(target)、时间/指标等上下文实体 |
| **意图识别** | 7种意图类型: create/query/update/delete/analyze/configure/integrate |
| **约束提取** | 时间、性能、规模、兼容性、安全、质量等6类约束 |
| **JSON输出** | 生成标准化结构化需求描述 |

## 输出JSON结构

```json
{
  "requirementId": "REQ-0001",
  "rawInput": "原始输入...",
  "timestamp": "2025-01-15T10:30:00",
  "entities": [
    { "name": "管理员", "type": "person", "role": "actor" },
    { "name": "用户管理模块", "type": "module", "role": "target" }
  ],
  "intent": {
    "action": "create",
    "target": "用户管理模块",
    "description": "创建用户管理模块",
    "confidence": 0.85
  },
  "constraints": [
    {
      "type": "time",
      "field": "responseTime",
      "operator": "lte",
      "value": "500ms",
      "description": "不超过500ms"
    }
  ],
  "summary": "管理员需要创建[用户管理模块]。"
}
```

## 编译与运行

### Windows
```batch
build.bat          # 自动检测编译器并构建
RequirementParser.exe
```

### Linux/macOS
```bash
mkdir build && cd build
cmake .. && make
./RequirementParser
```

### 手动编译
```bash
g++ -std=c++17 -O2 -o RequirementParser main.cpp RequirementParser.cpp
```

## 使用示例

### C++ API
```cpp
#include "RequirementParser.hpp"
using namespace RequirementParser;

Parser parser;
StandardRequirement req = parser.parse("管理员需要创建用户管理模块，响应时间不超过500ms");
std::cout << req.toPrettyJSON() << std::endl;
```

### 命令行交互
```
请输入需求> 系统需要支持100万用户同时在线，QPS达到5000
{ ...结构化JSON输出... }
```

## 意图类型说明

| 意图 | 中文关键词示例 |
|------|---------------|
| create | 创建、新建、构建、开发、生成、添加 |
| query | 查询、搜索、查找、获取、查看、显示 |
| update | 修改、更新、更改、调整、编辑 |
| delete | 删除、移除、清除、销毁 |
| analyze | 分析、评估、统计、计算、监控 |
| configure | 配置、设置、安装、部署、初始化 |
| integrate | 集成、对接、连接、同步、导入 |

## 约束类型说明

| 类型 | 示例 |
|------|------|
| time | 响应时间不超过500ms，在7天内完成 |
| performance | QPS达到5000，延迟小于200ms |
| scale | 支持100万用户，处理10亿条记录 |
| compatibility | 兼容Chrome浏览器，支持iOS/Android |
| security | 加密传输，权限分级，操作审计 |
| quality | 7x24小时可用，数据完整性校验 |
