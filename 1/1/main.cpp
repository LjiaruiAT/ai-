/**
 * 任务拆解引擎 - 完整演示
 *
 * 流水线： 自然语言 → RequirementParser → StandardRequirement
 *         → TaskDecompositionEngine → TaskGraph → ExecutionSchedule
 *
 * 编译： g++ -std=c++17 -O2 main.cpp RequirementParser.cpp TaskDecompositionEngine.cpp -o TaskDecomposer
 *
 * 运行： ./TaskDecomposer
 */

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>

#include "RequirementParser.hpp"
#include "TaskDecompositionEngine.hpp"

using namespace RequirementParser;
using namespace TaskDecomposition;

// ==================== 工具函数 ====================

void printSeparator(const std::string& title = "") {
    std::cout << "\n";
    if (!title.empty()) {
        std::cout << "══════ " << title << " ══════\n\n";
    } else {
        std::cout << "────────────────────────────────────────────────────────────\n";
    }
}

void printStage(const std::string& label, const std::string& content) {
    std::cout << "  [" << label << "] " << content << "\n";
}

// ==================== 场景1：创建系统 ====================

void demo1_createSystem(TaskDecompositionEngine& engine) {
    printSeparator("场景1：创建新系统");

    const std::string rawInput =
        "开发一个用户管理系统，支持用户注册、登录、权限管理。"
        "要求支持1000并发用户，响应时间小于200毫秒，"
        "需要支持HTTPS加密传输和SQL注入防护。";

    std::cout << "📝 原始需求：\n  \"" << rawInput << "\"\n\n";

    // 第一步：解析需求
    Parser parser;
    StandardRequirement req = parser.parse(rawInput);
    std::cout << "🔍 需求解析结果：\n";
    std::cout << "  需求ID: " << req.requirementId << "\n";
    std::cout << "  意图: " << req.intent.action << " (置信度: " << req.intent.confidence << ")\n";
    std::cout << "  目标: " << req.intent.target << "\n";
    std::cout << "  实体: ";
    for (const auto& e : req.entities) {
        std::cout << e.name << "(" << e.type << ") ";
    }
    std::cout << "\n";
    std::cout << "  约束: ";
    for (const auto& c : req.constraints) {
        std::cout << c.type << "=" << c.value << " ";
    }
    std::cout << "\n";
    std::cout << "  摘要: " << req.summary << "\n";

    // 第二步：分解为任务图
    DecompositionResult result = engine.decompose(req);

    printSeparator("分解结果");

    std::cout << "📊 应用规则: ";
    for (const auto& rid : result.appliedRules) {
        std::cout << rid << " ";
    }
    std::cout << "\n";
    std::cout << "📊 置信度: " << result.confidence << "\n";
    if (!result.warnings.empty()) {
        for (const auto& w : result.warnings) {
            std::cout << "⚠️  警告: " << w << "\n";
        }
    }

    printSeparator("任务列表");
    std::cout << std::left;
    std::cout << std::setw(12) << "任务ID"
              << std::setw(24) << "名称"
              << std::setw(14) << "分类"
              << std::setw(10) << "工时(h)"
              << std::setw(10) << "优先级"
              << "入度/出度\n";
    std::cout << std::string(85, '-') << "\n";

    // 按拓扑顺序显示
    auto topo = result.graph.topologicalSort();
    for (const auto& tid : topo) {
        const auto& task = result.graph.getTask(tid);
        int inDeg = result.graph.getInDegree(tid);
        int outDeg = result.graph.getOutDegree(tid);
        std::cout << std::setw(12) << tid
                  << std::setw(24) << task.name.substr(0, 22)
                  << std::setw(14) << task.category
                  << std::setw(10) << task.estimatedHours
                  << std::setw(10) << taskPriorityToString(task.priority)
                  << inDeg << "/" << outDeg << "\n";
    }

    printSeparator("依赖关系图 (拓扑顺序)");
    std::cout << "  共 " << result.graph.taskCount() << " 个任务，"
              << result.graph.dependencyCount() << " 条依赖\n\n";

    for (size_t i = 0; i < topo.size(); ++i) {
        const auto& task = result.graph.getTask(topo[i]);
        auto preds = result.graph.getPredecessors(topo[i]);
        if (preds.empty()) {
            std::cout << "  ▶ " << task.name << " (根任务，可立即开始)\n";
        } else {
            std::cout << "  ├─ " << task.name << "\n";
            for (const auto& pred : preds) {
                // 查找依赖类型
                std::string dtype = "hard";
                for (const auto& dep : result.graph.dependencies) {
                    if (dep.fromTaskId == pred && dep.toTaskId == topo[i]) {
                        dtype = dependencyTypeToString(dep.type);
                        break;
                    }
                }
                std::cout << "  │   ↑ 依赖: " << pred << " (" << dtype << ")\n";
            }
        }
        if (i < topo.size() - 1) std::cout << "  │\n";
    }

    printSeparator("执行调度计划");
    const auto& sched = result.schedule;
    std::cout << "  调度ID: " << sched.scheduleId << "\n";
    std::cout << "  总预估工时: " << sched.estimatedTotalHours << " 小时 ("
              << std::fixed << std::setprecision(1)
              << sched.estimatedTotalHours / 8.0 << " 人天)\n";
    std::cout << "  关键路径: " << sched.criticalPathHours << " 小时\n";
    std::cout << "  最大并行度: " << sched.maxParallelism << "\n\n";

    for (const auto& stage : sched.stages) {
        std::cout << "  阶段 " << stage.stageNumber << ": "
                  << stage.stageName << "\n";
        std::cout << "    " << stage.description << "\n";
        std::cout << "    并行任务 (" << stage.taskIds.size() << "): ";
        for (size_t j = 0; j < stage.taskIds.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << result.graph.getTask(stage.taskIds[j]).name;
        }
        std::cout << "\n";
    }

    // 甘特图
    printSeparator("文本甘特图");
    std::cout << sched.toGanttText();

    // 输出 Graphviz DOT
    printSeparator("Graphviz DOT (可复制到 viz-js.com 或 graphviz.org 查看)");
    std::cout << sched.sourceRequirementId << "_" << sched.scheduleId << ": 依赖图 DOT 输出如下\n\n";
    std::cout << result.graph.toDOT() << "\n";
}

// ==================== 场景2：系统集成 ====================

void demo2_integration(TaskDecompositionEngine& engine) {
    printSeparator("场景2：系统集成");

    const std::string rawInput =
        "将支付系统与第三方支付宝和微信支付进行对接集成，"
        "需要支持异步回调通知，保证数据一致性，"
        "要求支付接口响应时间小于1秒。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);

    std::cout << "📝 原始需求：\n  \"" << rawInput << "\"\n\n";
    std::cout << "  意图: " << req.intent.action
              << " | 置信度: " << req.intent.confidence << "\n";

    DecompositionResult result = engine.decompose(req);

    std::cout << "\n📊 命中规则: ";
    for (const auto& r : result.appliedRules) std::cout << r << " ";
    std::cout << "\n";

    auto topo = result.graph.topologicalSort();
    std::cout << "\n  任务流程 (" << result.graph.taskCount() << " 个任务):\n";
    for (const auto& tid : topo) {
        const auto& t = result.graph.getTask(tid);
        std::cout << "    " << tid << " → " << t.name
                  << " [" << t.estimatedHours << "h]\n";
    }

    std::cout << "\n📅 调度阶段:\n";
    for (const auto& stage : result.schedule.stages) {
        std::cout << "    阶段" << stage.stageNumber << ": ";
        for (size_t j = 0; j < stage.taskIds.size(); ++j) {
            if (j > 0) std::cout << " ‖ ";  // 并行符号
            std::cout << result.graph.getTask(stage.taskIds[j]).name;
        }
        std::cout << "\n";
    }

    std::cout << "\n⏱️  关键路径: " << result.schedule.criticalPathHours
              << "h / 总计: " << result.schedule.estimatedTotalHours << "h\n";
}

// ==================== 场景3：查询报表 ====================

void demo3_queryReport(TaskDecompositionEngine& engine) {
    printSeparator("场景3：查询报表系统");

    const std::string rawInput =
        "开发一个销售数据统计报表系统，支持按日、周、月、季度维度查询，"
        "需要导出Excel和PDF格式，数据量级为千万级，"
        "查询响应时间不超过3秒。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);
    DecompositionResult result = engine.decompose(req);

    std::cout << "📝 原始需求：\n  \"" << rawInput << "\"\n\n";
    std::cout << "  意图: " << req.intent.action << " | 目标: " << req.intent.target << "\n\n";

    auto groups = result.graph.findParallelGroups();
    std::cout << "⚡ 并行执行组 (" << groups.size() << " 层):\n";
    for (size_t i = 0; i < groups.size(); ++i) {
        std::cout << "  层级 " << (i + 1) << " (可并行 " << groups[i].size() << " 个):\n";
        for (const auto& tid : groups[i]) {
            std::cout << "    ▸ " << result.graph.getTask(tid).name
                      << " [" << result.graph.getTask(tid).estimatedHours << "h]\n";
        }
    }
}

// ==================== 场景4：功能更新 ====================

void demo4_updateFeature(TaskDecompositionEngine& engine) {
    printSeparator("场景4：功能更新/重构");

    const std::string rawInput =
        "重构用户认证模块，将Session-based认证迁移到JWT Token认证，"
        "需要兼容现有API接口，确保零停机迁移。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);
    DecompositionResult result = engine.decompose(req);

    std::cout << "📝 原始需求：\n  \"" << rawInput << "\"\n\n";
    std::cout << "  意图: " << req.intent.action << "\n";
    std::cout << "  命中规则: ";
    for (const auto& r : result.appliedRules) std::cout << r << " ";
    std::cout << "\n\n";

    // 显示带权重的关键路径
    auto critPath = result.graph.criticalPath();
    std::cout << "🔥 关键路径 (" << critPath.size() << " 个任务, "
              << result.graph.criticalPathDuration() << "h):\n";
    for (size_t i = 0; i < critPath.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << result.graph.getTask(critPath[i]).name
                  << " [" << result.graph.getTask(critPath[i]).estimatedHours << "h]\n";
    }
}

// ==================== 场景5：自定义规则 ====================

void demo5_customRule(TaskDecompositionEngine& engine) {
    printSeparator("场景5：自定义分解规则");

    // 创建自定义规则：技术调研
    DecompositionRule customRule;
    customRule.ruleId = "CUSTOM-RESEARCH";
    customRule.ruleName = "技术调研专用流程";
    customRule.intentAction = "analyze";
    customRule.entityType = "system";  // 只匹配系统相关的分析

    customRule.templateTasks = {
        {"", "技术背景调研", "调研相关技术栈、行业方案、竞品分析",
         "analysis", 8.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"research"}, {"信息检索"}},
        {"", "可行性分析", "从技术、成本、时间维度评估可行性",
         "analysis", 6.0, TaskPriority::HIGH, TaskStatus::PENDING, "", {"feasibility"}, {"分析能力"}},
        {"", "POC原型验证", "搭建最小可行原型验证核心技术点",
         "implementation", 12.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"poc"}, {"快速原型"}},
        {"", "技术选型报告", "输出技术选型建议及对比矩阵",
         "documentation", 6.0, TaskPriority::MEDIUM, TaskStatus::PENDING, "", {"report"}, {"技术写作"}},
    };
    customRule.templateDependencies = {
        {0, 1}, {1, 2}, {2, 3},
    };

    engine.addRule(customRule);
    std::cout << "✅ 已注册自定义规则: " << customRule.ruleId
              << " - " << customRule.ruleName << "\n\n";

    // 测试自定义规则
    const std::string rawInput =
        "调研市场上主流的消息队列系统（Kafka、RabbitMQ、Pulsar），"
        "评估其在本系统场景下的适用性。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);
    DecompositionResult result = engine.decompose(req);

    std::cout << "📝 需求: \"" << rawInput << "\"\n";
    std::cout << "  意图: " << req.intent.action << "\n";
    std::cout << "  命中规则: ";
    for (const auto& r : result.appliedRules) std::cout << r << " ";
    std::cout << "\n\n";

    for (const auto& kv : result.graph.tasks) {
        std::cout << "  " << kv.second.taskId << " | "
                  << std::setw(20) << std::left << kv.second.name
                  << " | " << kv.second.estimatedHours << "h\n";
    }
}

// ==================== 场景6：完整JSON输出 ====================

void demo6_fullJSON() {
    printSeparator("场景6：完整JSON输出");

    const std::string rawInput =
        "创建一个自动化CI/CD流水线，支持代码提交自动构建、测试、部署到Kubernetes集群，"
        "构建时间不超过10分钟，需要集成SonarQube代码质量检查。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);

    TaskDecompositionEngine engine;
    DecompositionResult result = engine.decompose(req);

    std::cout << "📋 需求解析 JSON:\n";
    std::cout << req.toPrettyJSON() << "\n\n";

    std::cout << "📋 完整分解结果 JSON:\n";
    std::cout << result.toJSON() << "\n";
}

// ==================== 场景7：拓扑排序与并行能力分析 ====================

void demo7_parallelism(TaskDecompositionEngine& engine) {
    printSeparator("场景7：并行能力分析");

    const std::string rawInput =
        "开发一个电商平台，包含商品管理、订单系统、支付集成、"
        "用户中心、物流跟踪、评价系统六大模块。"
        "要求支持日均百万订单，系统可用性99.99%。";

    Parser parser;
    StandardRequirement req = parser.parse(rawInput);

    // 添加更多自定义规则来模拟复杂场景
    {
        DecompositionRule shopRule;
        shopRule.ruleId = "CUSTOM-ESHOP";
        shopRule.ruleName = "电商平台多模块开发";
        shopRule.intentAction = "create";
        shopRule.entityType = "system";

        shopRule.templateTasks = {
            {"", "商品管理模块开发", "实现商品CRUD、分类、搜索、库存管理",
             "implementation", 40.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"module"}, {"后端开发"}},
            {"", "订单系统开发", "实现订单创建、状态流转、库存扣减、退款",
             "implementation", 48.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"module"}, {"后端开发"}},
            {"", "支付集成模块", "对接支付宝、微信、银联支付网关",
             "integration", 32.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"module"}, {"支付"}},
            {"", "用户中心开发", "用户注册登录、会员体系、地址管理",
             "implementation", 32.0, TaskPriority::HIGH, TaskStatus::PENDING, "", {"module"}, {"后端开发"}},
            {"", "物流跟踪模块", "对接快递鸟/菜鸟接口，实现物流状态追踪",
             "integration", 24.0, TaskPriority::MEDIUM, TaskStatus::PENDING, "", {"module"}, {"API对接"}},
            {"", "评价系统开发", "商品评价、追评、晒图、好评率统计",
             "implementation", 20.0, TaskPriority::MEDIUM, TaskStatus::PENDING, "", {"module"}, {"后端开发"}},
            {"", "架构设计", "微服务架构设计、服务拆分、通信协议、网关设计",
             "design", 24.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"architecture"}, {"架构能力"}},
            {"", "数据库设计", "分库分表策略、读写分离、缓存架构",
             "design", 20.0, TaskPriority::CRITICAL, TaskStatus::PENDING, "", {"database"}, {"数据建模"}},
            {"", "系统集成测试", "全链路测试、压力测试、混沌工程",
             "testing", 24.0, TaskPriority::HIGH, TaskStatus::PENDING, "", {"testing"}, {"测试"}},
            {"", "部署上线", "K8s部署、灰度发布、监控告警配置",
             "deployment", 16.0, TaskPriority::HIGH, TaskStatus::PENDING, "", {"deployment"}, {"DevOps"}},
        };

        // 架构设计 → 各模块 → 集成测试 → 部署
        shopRule.templateDependencies = {
            {6, 0}, {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5},  // 架构→模块
            {7, 0}, {7, 1},                                     // 数据库→商品、订单
            {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8},   // 模块→集成测试
            {8, 9},                                             // 集成测试→部署
        };

        engine.addRule(shopRule);
    }

    DecompositionResult result = engine.decompose(req);

    std::cout << "📊 任务总数: " << result.graph.taskCount() << "\n";
    std::cout << "📊 依赖边数: " << result.graph.dependencyCount() << "\n\n";

    auto groups = result.graph.findParallelGroups();

    // 可视化并行度
    const int barMax = 60;
    std::cout << "📈 并行度分布:\n\n";
    for (size_t i = 0; i < groups.size(); ++i) {
        int n = (int)groups[i].size();
        int barLen = (n * barMax) / (std::max)(1, result.schedule.maxParallelism);

        std::cout << "  层级" << std::setw(2) << (i + 1) << " │ ";
        std::cout << std::string(barLen, '█')
                  << std::string(barMax - barLen, '░')
                  << " │ " << n << " 并行";
        std::cout << "\n";
    }

    std::cout << "\n📐 最大并行度: " << result.schedule.maxParallelism << "\n";
    std::cout << "📐 层级数(深度): " << groups.size() << "\n";

    double speedup = result.schedule.estimatedTotalHours /
                     (std::max)(1.0, result.schedule.criticalPathHours);
    std::cout << "📐 理论加速比: " << std::fixed << std::setprecision(2)
              << speedup << "x (总工时/关键路径)\n";

    // 找出根任务和叶子任务
    auto roots = result.graph.getRootTasks();
    auto leaves = result.graph.getLeafTasks();
    std::cout << "\n🌱 根任务 (可立即启动, " << roots.size() << "个):\n";
    for (const auto& rid : roots) {
        std::cout << "    → " << result.graph.getTask(rid).name << "\n";
    }
    std::cout << "🍂 叶子任务 (无后继, " << leaves.size() << "个):\n";
    for (const auto& lid : leaves) {
        std::cout << "    → " << result.graph.getTask(lid).name << "\n";
    }

    // Mermaid
    printSeparator("Mermaid 图 (可复制到 mermaid.live 查看)");
    std::cout << result.graph.toMermaid() << "\n";
}

// ==================== main ====================

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║           任务拆解引擎 (Task Decomposition Engine)           ║
║   高层需求 → 原子子任务 → 依赖图 → 串行/并行调度           ║
╚══════════════════════════════════════════════════════════════╝
)";

    TaskDecompositionEngine engine;

    try {
        demo1_createSystem(engine);
        demo2_integration(engine);
        demo3_queryReport(engine);
        demo4_updateFeature(engine);
        demo5_customRule(engine);
        demo6_fullJSON();
        demo7_parallelism(engine);
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 出错: " << e.what() << "\n";
        return 1;
    }

    printSeparator("全部演示完成 ✅");
    std::cout << "\n引擎能力总结：\n";
    std::cout << "  ✓ 自然语言需求解析（实体、意图、约束提取）\n";
    std::cout << "  ✓ 7类内置分解规则（create/query/update/delete/analyze/configure/integrate）\n";
    std::cout << "  ✓ 自定义规则注册与优先级匹配\n";
    std::cout << "  ✓ 任务依赖图构建（HARD/SOFT依赖）\n";
    std::cout << "  ✓ 拓扑排序 (Kahn算法)\n";
    std::cout << "  ✓ 并行执行组划分（层级分组）\n";
    std::cout << "  ✓ 关键路径分析（CPM正向/逆向传播）\n";
    std::cout << "  ✓ 环检测与自动处理\n";
    std::cout << "  ✓ 约束驱动的工时估算与优先级推断\n";
    std::cout << "  ✓ 横切关注点注入（项目管理、代码审查、安全审计）\n";
    std::cout << "  ✓ 多格式导出（JSON / DOT / Mermaid / PlantUML / 文本甘特图）\n";
    std::cout << "  ✓ 并行/串行混合调度计划生成\n\n";

    return 0;
}
