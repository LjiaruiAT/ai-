#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <iomanip>

// ==================== 前向声明：依赖 RequirementParser ====================
// 实际项目中 #include "RequirementParser.hpp"，此处内联所需结构体以保持独立编译

namespace RequirementParser {

struct Entity {
    std::string name;
    std::string type;       // person, system, module, data, time, location, metric
    std::string role;       // actor, target, context
};

struct Intent {
    std::string action;      // create, query, update, delete, analyze, configure, integrate
    std::string target;
    std::string description;
    double confidence;
};

struct Constraint {
    std::string type;        // time, performance, budget, compatibility, security, scale, quality
    std::string field;
    std::string operator_;   // eq, lt, gt, lte, gte, range, contains
    std::string value;
    std::string description;
};

struct StandardRequirement {
    std::string requirementId;
    std::string rawInput;
    std::vector<Entity> entities;
    Intent intent;
    std::vector<Constraint> constraints;
    std::string summary;
    std::string timestamp;
};

} // namespace RequirementParser

// ============================================================
//  TaskDecomposition 命名空间
// ============================================================
namespace TaskDecomposition {

// ==================== 枚举 ====================

enum class TaskStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    BLOCKED,
    CANCELLED
};

inline std::string taskStatusToString(TaskStatus s) {
    switch (s) {
        case TaskStatus::PENDING:     return "pending";
        case TaskStatus::IN_PROGRESS: return "in_progress";
        case TaskStatus::COMPLETED:   return "completed";
        case TaskStatus::BLOCKED:     return "blocked";
        case TaskStatus::CANCELLED:   return "cancelled";
    }
    return "unknown";
}

inline TaskStatus taskStatusFromString(const std::string& s) {
    if (s == "pending")      return TaskStatus::PENDING;
    if (s == "in_progress")  return TaskStatus::IN_PROGRESS;
    if (s == "completed")    return TaskStatus::COMPLETED;
    if (s == "blocked")      return TaskStatus::BLOCKED;
    if (s == "cancelled")    return TaskStatus::CANCELLED;
    return TaskStatus::PENDING;
}

enum class TaskPriority {
    CRITICAL = 1,
    HIGH = 2,
    MEDIUM = 3,
    LOW = 4,
    OPTIONAL = 5
};

inline std::string taskPriorityToString(TaskPriority p) {
    switch (p) {
        case TaskPriority::CRITICAL: return "critical";
        case TaskPriority::HIGH:     return "high";
        case TaskPriority::MEDIUM:   return "medium";
        case TaskPriority::LOW:      return "low";
        case TaskPriority::OPTIONAL: return "optional";
    }
    return "medium";
}

inline TaskPriority taskPriorityFromString(const std::string& s) {
    if (s == "critical") return TaskPriority::CRITICAL;
    if (s == "high")     return TaskPriority::HIGH;
    if (s == "medium")   return TaskPriority::MEDIUM;
    if (s == "low")      return TaskPriority::LOW;
    if (s == "optional") return TaskPriority::OPTIONAL;
    return TaskPriority::MEDIUM;
}

enum class DependencyType {
    HARD,   // 强依赖：前驱必须完成后继才能开始
    SOFT    // 软依赖：前驱应该完成后继再开始（可并行）
};

inline std::string dependencyTypeToString(DependencyType d) {
    return (d == DependencyType::HARD) ? "hard" : "soft";
}

// ==================== 原子任务 ====================

struct AtomicTask {
    std::string taskId;                   // 唯一标识
    std::string name;                     // 任务名称
    std::string description;              // 详细描述
    std::string category;                 // 分类：analysis, design, implementation, testing,
                                          //        deployment, documentation, review, integration
    double estimatedHours;               // 预估工时（小时）
    TaskPriority priority;               // 优先级
    TaskStatus status;                   // 状态
    std::string assignedTo;              // 指派人（空 = 未分配）
    std::vector<std::string> tags;       // 标签
    std::vector<std::string> requiredSkills; // 所需技能

    std::string toJSON() const;
};

// ==================== 任务依赖 ====================

struct TaskDependency {
    std::string fromTaskId;    // 前驱任务
    std::string toTaskId;      // 后继任务
    DependencyType type;       // 依赖类型
    std::string reason;        // 依赖原因

    std::string toJSON() const;
};

// ==================== 任务依赖图 ====================

struct TaskGraph {
    std::string graphId;                        // 图ID
    std::string sourceRequirementId;            // 来源需求ID
    std::map<std::string, AtomicTask> tasks;    // taskId → 任务
    std::vector<TaskDependency> dependencies;   // 依赖列表

    // ---------- 基础操作 ----------
    void addTask(const AtomicTask& task);
    void addDependency(const std::string& from, const std::string& to,
                       DependencyType type = DependencyType::HARD,
                       const std::string& reason = "");
    bool hasTask(const std::string& taskId) const;
    AtomicTask& getTask(const std::string& taskId);
    const AtomicTask& getTask(const std::string& taskId) const;
    size_t taskCount() const;
    size_t dependencyCount() const;

    // ---------- 图分析 ----------
    std::vector<std::string> getPredecessors(const std::string& taskId) const;
    std::vector<std::string> getSuccessors(const std::string& taskId) const;
    std::vector<std::string> getRootTasks() const;     // 无前驱的任务
    std::vector<std::string> getLeafTasks() const;     // 无后继的任务
    int getInDegree(const std::string& taskId) const;
    int getOutDegree(const std::string& taskId) const;

    // ---------- 拓扑排序 ----------
    std::vector<std::string> topologicalSort() const;  // Kahn算法

    // ---------- 并行执行组 ----------
    // 将拓扑排序按层级分组，同层任务可并行
    std::vector<std::vector<std::string>> findParallelGroups() const;

    // ---------- 环检测 ----------
    bool hasCycle() const;
    std::vector<std::string> findCycle() const;        // 返回环上的任务ID列表

    // ---------- 关键路径 ----------
    std::vector<std::string> criticalPath() const;
    double criticalPathDuration() const;

    // ---------- 序列化 ----------
    std::string toJSON() const;
    std::string toDOT() const;       // Graphviz DOT 格式
    std::string toMermaid() const;   // Mermaid.js 格式
};

// ==================== 调度阶段 ====================

struct ScheduleStage {
    int stageNumber;                          // 阶段编号（从1开始）
    std::string stageName;                    // 阶段名称
    std::vector<std::string> taskIds;         // 本阶段可并行的任务ID
    std::string description;                  // 阶段描述

    std::string toJSON() const;
};

// ==================== 执行调度计划 ====================

struct ExecutionSchedule {
    std::string scheduleId;                   // 调度ID
    std::string sourceRequirementId;          // 来源需求ID
    std::vector<ScheduleStage> stages;        // 调度阶段列表
    double estimatedTotalHours;               // 总预估工时（所有任务之和）
    double criticalPathHours;                 // 关键路径时长
    int maxParallelism;                       // 最大并行度

    std::string toJSON() const;
    std::string toGanttText() const;          // 文本甘特图
    std::string toPlantUML() const;           // PlantUML 甘特图
};

// ==================== 分解规则 ====================

struct DecompositionRule {
    std::string ruleId;                                       // 规则ID
    std::string ruleName;                                     // 规则名称
    std::string intentAction;                                 // 匹配的意图动作（空 = 匹配所有）
    std::string entityType;                                   // 匹配的实体类型（空 = 匹配所有）
    std::vector<AtomicTask> templateTasks;                    // 模板任务列表
    // 模板依赖 (fromIndex, toIndex)，索引对应 templateTasks 的下标
    std::vector<std::pair<size_t, size_t>> templateDependencies;

    /** 判断规则是否匹配给定的 action 和 entityType */
    bool matches(const std::string& action, const std::string& eType) const;
};

// ==================== 分解结果元数据 ====================

struct DecompositionResult {
    TaskGraph graph;
    ExecutionSchedule schedule;
    std::vector<std::string> appliedRules;    // 命中的规则ID列表
    double confidence;                        // 分解置信度
    std::vector<std::string> warnings;        // 警告信息

    std::string toJSON() const;
};

// ==================== 任务分解引擎 ====================

class TaskDecompositionEngine {
public:
    TaskDecompositionEngine();

    /**
     * 将标准化需求分解为任务依赖图
     * @param requirement 标准化需求描述
     * @return DecompositionResult 包含任务图和调度计划
     */
    DecompositionResult decompose(
        const RequirementParser::StandardRequirement& requirement);

    /**
     * 仅分解为任务图（不生成调度）
     */
    TaskGraph decomposeToGraph(
        const RequirementParser::StandardRequirement& requirement);

    /**
     * 基于任务图生成执行调度
     */
    ExecutionSchedule schedule(const TaskGraph& graph);

    /**
     * 添加自定义分解规则
     */
    void addRule(const DecompositionRule& rule);

    /**
     * 加载内置默认规则
     */
    void loadDefaultRules();

    /**
     * 获取所有已注册规则
     */
    const std::vector<DecompositionRule>& getRules() const { return rules_; }

    /**
     * 清空所有规则
     */
    void clearRules() { rules_.clear(); }

    /**
     * 设置全局工时倍率（根据团队能力调整）
     */
    void setEffortMultiplier(double multiplier) { effortMultiplier_ = multiplier; }
    double getEffortMultiplier() const { return effortMultiplier_; }

private:
    std::vector<DecompositionRule> rules_;
    int taskCounter_;
    int graphCounter_;
    int scheduleCounter_;
    double effortMultiplier_;

    // ---------- ID生成 ----------
    std::string generateTaskId();
    std::string generateGraphId();
    std::string generateScheduleId();

    // ---------- 规则匹配 ----------
    struct RuleMatch {
        const DecompositionRule* rule;
        double score;   // 匹配得分
    };
    std::vector<RuleMatch> matchRules(
        const RequirementParser::StandardRequirement& requirement) const;

    // ---------- 规则应用 ----------
    TaskGraph applyRule(const DecompositionRule& rule,
                        const RequirementParser::StandardRequirement& requirement,
                        std::map<std::string, std::string>& idMapping);

    // ---------- 图合并 ----------
    void mergeGraph(TaskGraph& target, const TaskGraph& source);

    // ---------- 横切关注点 ----------
    void addCrossCuttingTasks(TaskGraph& graph,
                              const RequirementParser::StandardRequirement& req);

    // ---------- 依赖推断 ----------
    void inferDependencies(TaskGraph& graph,
                           const RequirementParser::StandardRequirement& req);

    // ---------- 约束驱动的工时估算 ----------
    double estimateEffort(const RequirementParser::StandardRequirement& req,
                          const AtomicTask& taskTemplate) const;
    double getConstraintMultiplier(const RequirementParser::StandardRequirement& req) const;

    // ---------- 任务优先级推断 ----------
    TaskPriority inferPriority(const AtomicTask& task,
                               const RequirementParser::StandardRequirement& req) const;
};

} // namespace TaskDecomposition
