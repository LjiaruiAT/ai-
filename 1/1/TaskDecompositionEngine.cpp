#include "TaskDecompositionEngine.hpp"
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>
#include <cassert>

namespace TaskDecomposition {

// ==================== AtomicTask JSON ====================

std::string AtomicTask::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"taskId\":\"" << taskId << "\","
        << "\"name\":\"" << name << "\","
        << "\"description\":\"" << description << "\","
        << "\"category\":\"" << category << "\","
        << "\"estimatedHours\":" << estimatedHours << ","
        << "\"priority\":\"" << taskPriorityToString(priority) << "\","
        << "\"status\":\"" << taskStatusToString(status) << "\","
        << "\"assignedTo\":\"" << assignedTo << "\","
        << "\"tags\":[";
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << tags[i] << "\"";
    }
    oss << "],\"requiredSkills\":[";
    for (size_t i = 0; i < requiredSkills.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << requiredSkills[i] << "\"";
    }
    oss << "]}";
    return oss.str();
}

// ==================== TaskDependency JSON ====================

std::string TaskDependency::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"from\":\"" << fromTaskId << "\","
        << "\"to\":\"" << toTaskId << "\","
        << "\"type\":\"" << dependencyTypeToString(type) << "\","
        << "\"reason\":\"" << reason << "\""
        << "}";
    return oss.str();
}

// ==================== TaskGraph ====================

void TaskGraph::addTask(const AtomicTask& task) {
    tasks[task.taskId] = task;
}

void TaskGraph::addDependency(const std::string& from, const std::string& to,
                               DependencyType type, const std::string& reason) {
    TaskDependency dep;
    dep.fromTaskId = from;
    dep.toTaskId = to;
    dep.type = type;
    dep.reason = reason;
    dependencies.push_back(dep);
}

bool TaskGraph::hasTask(const std::string& taskId) const {
    return tasks.find(taskId) != tasks.end();
}

AtomicTask& TaskGraph::getTask(const std::string& taskId) {
    return tasks.at(taskId);
}

const AtomicTask& TaskGraph::getTask(const std::string& taskId) const {
    return tasks.at(taskId);
}

size_t TaskGraph::taskCount() const { return tasks.size(); }
size_t TaskGraph::dependencyCount() const { return dependencies.size(); }

std::vector<std::string> TaskGraph::getPredecessors(const std::string& taskId) const {
    std::vector<std::string> preds;
    for (const auto& dep : dependencies) {
        if (dep.toTaskId == taskId) {
            preds.push_back(dep.fromTaskId);
        }
    }
    return preds;
}

std::vector<std::string> TaskGraph::getSuccessors(const std::string& taskId) const {
    std::vector<std::string> succs;
    for (const auto& dep : dependencies) {
        if (dep.fromTaskId == taskId) {
            succs.push_back(dep.toTaskId);
        }
    }
    return succs;
}

std::vector<std::string> TaskGraph::getRootTasks() const {
    std::set<std::string> hasPred;
    for (const auto& dep : dependencies) {
        hasPred.insert(dep.toTaskId);
    }
    std::vector<std::string> roots;
    for (const auto& kv : tasks) {
        if (hasPred.find(kv.first) == hasPred.end()) {
            roots.push_back(kv.first);
        }
    }
    return roots;
}

std::vector<std::string> TaskGraph::getLeafTasks() const {
    std::set<std::string> hasSucc;
    for (const auto& dep : dependencies) {
        hasSucc.insert(dep.fromTaskId);
    }
    std::vector<std::string> leaves;
    for (const auto& kv : tasks) {
        if (hasSucc.find(kv.first) == hasSucc.end()) {
            leaves.push_back(kv.first);
        }
    }
    return leaves;
}

int TaskGraph::getInDegree(const std::string& taskId) const {
    int count = 0;
    for (const auto& dep : dependencies) {
        if (dep.toTaskId == taskId) ++count;
    }
    return count;
}

int TaskGraph::getOutDegree(const std::string& taskId) const {
    int count = 0;
    for (const auto& dep : dependencies) {
        if (dep.fromTaskId == taskId) ++count;
    }
    return count;
}

// ---------- Kahn 拓扑排序 ----------

std::vector<std::string> TaskGraph::topologicalSort() const {
    std::map<std::string, int> inDegree;
    for (const auto& kv : tasks) {
        inDegree[kv.first] = getInDegree(kv.first);
    }

    std::queue<std::string> q;
    for (const auto& kv : inDegree) {
        if (kv.second == 0) q.push(kv.first);
    }

    std::vector<std::string> result;
    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& dep : dependencies) {
        adj[dep.fromTaskId].push_back(dep.toTaskId);
    }

    while (!q.empty()) {
        std::string u = q.front(); q.pop();
        result.push_back(u);
        for (const auto& v : adj[u]) {
            if (--inDegree[v] == 0) {
                q.push(v);
            }
        }
    }

    return result;
}

// ---------- 并行执行组（按层级分组） ----------

std::vector<std::vector<std::string>> TaskGraph::findParallelGroups() const {
    std::map<std::string, int> inDegree;
    for (const auto& kv : tasks) {
        inDegree[kv.first] = getInDegree(kv.first);
    }

    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& dep : dependencies) {
        adj[dep.fromTaskId].push_back(dep.toTaskId);
    }

    std::vector<std::vector<std::string>> groups;
    std::queue<std::string> currentLevel;

    for (const auto& kv : inDegree) {
        if (kv.second == 0) currentLevel.push(kv.first);
    }

    while (!currentLevel.empty()) {
        std::vector<std::string> group;
        std::queue<std::string> nextLevel;

        while (!currentLevel.empty()) {
            std::string u = currentLevel.front(); currentLevel.pop();
            group.push_back(u);
            for (const auto& v : adj[u]) {
                if (--inDegree[v] == 0) {
                    nextLevel.push(v);
                }
            }
        }

        groups.push_back(group);
        currentLevel = nextLevel;
    }

    return groups;
}

// ---------- 环检测 ----------

bool TaskGraph::hasCycle() const {
    return topologicalSort().size() != tasks.size();
}

std::vector<std::string> TaskGraph::findCycle() const {
    // DFS 三色标记法：0=未访问, 1=访问中, 2=已完成
    std::map<std::string, int> color;
    std::map<std::string, std::string> parent;
    std::vector<std::string> cycle;

    std::map<std::string, std::vector<std::string>> adj;
    for (const auto& dep : dependencies) {
        adj[dep.fromTaskId].push_back(dep.toTaskId);
    }

    std::function<bool(const std::string&)> dfs = [&](const std::string& u) -> bool {
        color[u] = 1;
        for (const auto& v : adj[u]) {
            if (color[v] == 1) {
                // 找到环，回溯
                cycle.push_back(v);
                for (std::string cur = u; cur != v; cur = parent[cur]) {
                    cycle.push_back(cur);
                }
                cycle.push_back(v);
                std::reverse(cycle.begin(), cycle.end());
                return true;
            }
            if (color[v] == 0) {
                parent[v] = u;
                if (dfs(v)) return true;
            }
        }
        color[u] = 2;
        return false;
    };

    for (const auto& kv : tasks) {
        color[kv.first] = 0;
    }
    for (const auto& kv : tasks) {
        if (color[kv.first] == 0) {
            if (dfs(kv.first)) return cycle;
        }
    }
    return cycle;
}

// ---------- 关键路径 (CPM) ----------

std::vector<std::string> TaskGraph::criticalPath() const {
    if (tasks.empty()) return {};

    // 计算最早开始时间 (EST) 和最晚开始时间 (LST)
    std::map<std::string, double> est;  // earliest start
    std::map<std::string, double> eft;  // earliest finish
    std::map<std::string, double> lst;  // latest start
    std::map<std::string, double> lft;  // latest finish

    for (const auto& kv : tasks) {
        est[kv.first] = 0;
        eft[kv.first] = 0;
        lst[kv.first] = 1e18;
        lft[kv.first] = 1e18;
    }

    std::map<std::string, std::vector<std::string>> adj;
    std::map<std::string, std::vector<std::string>> revAdj;
    for (const auto& dep : dependencies) {
        adj[dep.fromTaskId].push_back(dep.toTaskId);
        revAdj[dep.toTaskId].push_back(dep.fromTaskId);
    }

    // 前向传递
    auto topo = topologicalSort();
    if (topo.size() != tasks.size()) return {}; // 有环

    for (const auto& u : topo) {
        eft[u] = est[u] + tasks.at(u).estimatedHours;
        for (const auto& v : adj[u]) {
            est[v] = std::max(est[v], eft[u]);
        }
    }

    // 找到最大完成时间
    double maxEft = 0;
    for (const auto& kv : tasks) {
        maxEft = std::max(maxEft, eft[kv.first]);
    }

    // 后向传递
    for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
        const std::string& u = *it;
        if (adj[u].empty()) {
            lft[u] = maxEft;
        }
        lst[u] = lft[u] - tasks.at(u).estimatedHours;
        for (const auto& p : revAdj[u]) {
            lft[p] = std::min(lft[p], lst[u]);
        }
    }

    // 关键路径：est == lst 的任务
    std::vector<std::string> cp;
    for (const auto& u : topo) {
        if (std::abs(est[u] - lst[u]) < 0.001) {
            cp.push_back(u);
        }
    }
    return cp;
}

double TaskGraph::criticalPathDuration() const {
    auto cp = criticalPath();
    if (cp.empty()) {
        // fallback: 取最长路径
        double maxDur = 0;
        for (const auto& kv : tasks) {
            maxDur = std::max(maxDur, kv.second.estimatedHours);
        }
        return maxDur;
    }
    double total = 0;
    for (const auto& tid : cp) {
        total += tasks.at(tid).estimatedHours;
    }
    return total;
}

// ---------- JSON ----------

std::string TaskGraph::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"graphId\":\"" << graphId << "\","
        << "\"sourceRequirementId\":\"" << sourceRequirementId << "\","
        << "\"tasks\":{";
    bool firstTask = true;
    for (const auto& kv : tasks) {
        if (!firstTask) oss << ",";
        firstTask = false;
        oss << "\"" << kv.first << "\":" << kv.second.toJSON();
    }
    oss << "},"
        << "\"dependencies\":[";
    for (size_t i = 0; i < dependencies.size(); ++i) {
        if (i > 0) oss << ",";
        oss << dependencies[i].toJSON();
    }
    oss << "]}";
    return oss.str();
}

std::string TaskGraph::toDOT() const {
    std::ostringstream oss;
    oss << "digraph TaskGraph {\n";
    oss << "  rankdir=TB;\n";
    oss << "  label=\"" << sourceRequirementId << "\";\n";
    oss << "  labelloc=t;\n";
    oss << "  node [shape=box, style=rounded];\n\n";

    for (const auto& kv : tasks) {
        oss << "  \"" << kv.first << "\" [label=\"" << kv.second.name
            << "\\n(" << kv.second.estimatedHours << "h)\"];\n";
    }

    oss << "\n";
    for (const auto& dep : dependencies) {
        oss << "  \"" << dep.fromTaskId << "\" -> \"" << dep.toTaskId << "\"";
        if (dep.type == DependencyType::SOFT) {
            oss << " [style=dashed]";
        }
        oss << ";\n";
    }
    oss << "}\n";
    return oss.str();
}

std::string TaskGraph::toMermaid() const {
    std::ostringstream oss;
    oss << "```mermaid\ngraph TB\n";
    for (const auto& kv : tasks) {
        oss << "  " << kv.first << "[\"" << kv.second.name
            << "<br/>" << kv.second.estimatedHours << "h\"]\n";
    }
    for (const auto& dep : dependencies) {
        oss << "  " << dep.fromTaskId << " -->";
        if (dep.type == DependencyType::SOFT) oss << "|soft|";
        oss << dep.toTaskId << "\n";
    }
    oss << "```\n";
    return oss.str();
}

// ==================== ScheduleStage JSON ====================

std::string ScheduleStage::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"stageNumber\":" << stageNumber << ","
        << "\"stageName\":\"" << stageName << "\","
        << "\"taskIds\":[";
    for (size_t i = 0; i < taskIds.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << taskIds[i] << "\"";
    }
    oss << "],"
        << "\"description\":\"" << description << "\""
        << "}";
    return oss.str();
}

// ==================== ExecutionSchedule ====================

std::string ExecutionSchedule::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"scheduleId\":\"" << scheduleId << "\","
        << "\"sourceRequirementId\":\"" << sourceRequirementId << "\","
        << "\"stages\":[";
    for (size_t i = 0; i < stages.size(); ++i) {
        if (i > 0) oss << ",";
        oss << stages[i].toJSON();
    }
    oss << "],"
        << "\"estimatedTotalHours\":" << estimatedTotalHours << ","
        << "\"criticalPathHours\":" << criticalPathHours << ","
        << "\"maxParallelism\":" << maxParallelism
        << "}";
    return oss.str();
}

std::string ExecutionSchedule::toGanttText() const {
    std::ostringstream oss;
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║  文本甘特图 (Text Gantt Chart)                              ║\n";
    oss << "╠══════════════════════════════════════════════════════════════╣\n";
    oss << "║  总工时: " << std::setw(6) << estimatedTotalHours << " h  |  关键路径: "
        << std::setw(6) << criticalPathHours << " h  |  最大并行度: "
        << std::setw(3) << maxParallelism << " ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n\n";

    for (const auto& stage : stages) {
        oss << "【阶段 " << stage.stageNumber << "】" << stage.stageName
            << "  (" << stage.taskIds.size() << " 个任务可并行)\n";
        oss << std::string(60, '─') << "\n";
        for (const auto& tid : stage.taskIds) {
            oss << "  │  " << tid << "\n";
        }
        oss << "\n";
    }
    return oss.str();
}

std::string ExecutionSchedule::toPlantUML() const {
    std::ostringstream oss;
    oss << "@startgantt\n";
    oss << "title Execution Schedule - " << sourceRequirementId << "\n";
    oss << "printscale daily\n\n";

    double dayOffset = 0;
    for (const auto& stage : stages) {
        for (const auto& tid : stage.taskIds) {
            // 简化：每个任务按天显示
            oss << "[" << tid << "] starts " << (int)dayOffset
                << " days from start and lasts 1 days\n";
        }
        dayOffset += 1;
    }
    oss << "@endgantt\n";
    return oss.str();
}

// ==================== DecompositionResult JSON ====================

std::string DecompositionResult::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"graph\":" << graph.toJSON() << ","
        << "\"schedule\":" << schedule.toJSON() << ","
        << "\"appliedRules\":[";
    for (size_t i = 0; i < appliedRules.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << appliedRules[i] << "\"";
    }
    oss << "],"
        << "\"confidence\":" << confidence << ","
        << "\"warnings\":[";
    for (size_t i = 0; i < warnings.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << warnings[i] << "\"";
    }
    oss << "]}";
    return oss.str();
}

// ==================== DecompositionRule ====================

bool DecompositionRule::matches(const std::string& action, const std::string& eType) const {
    bool actionMatch = intentAction.empty() || intentAction == action;
    bool typeMatch = entityType.empty() || entityType == eType;
    return actionMatch && typeMatch;
}

// ==================== TaskDecompositionEngine ====================

TaskDecompositionEngine::TaskDecompositionEngine()
    : taskCounter_(0), graphCounter_(0), scheduleCounter_(0), effortMultiplier_(1.0)
{
    loadDefaultRules();
}

// ---------- ID 生成 ----------

std::string TaskDecompositionEngine::generateTaskId() {
    std::ostringstream oss;
    oss << "TASK-" << std::setw(4) << std::setfill('0') << (++taskCounter_);
    return oss.str();
}

std::string TaskDecompositionEngine::generateGraphId() {
    std::ostringstream oss;
    oss << "GRAPH-" << std::setw(4) << std::setfill('0') << (++graphCounter_);
    return oss.str();
}

std::string TaskDecompositionEngine::generateScheduleId() {
    std::ostringstream oss;
    oss << "SCHED-" << std::setw(4) << std::setfill('0') << (++scheduleCounter_);
    return oss.str();
}

// ---------- 主入口 ----------

DecompositionResult TaskDecompositionEngine::decompose(
    const RequirementParser::StandardRequirement& requirement)
{
    DecompositionResult result;
    result.confidence = 0.0;

    // Step 1: 规则匹配
    auto matches = matchRules(requirement);

    if (matches.empty()) {
        result.warnings.push_back("No matching decomposition rules found for intent='"
            + requirement.intent.action + "'. Using generic fallback.");
        // Fallback: 使用通用规则
        DecompositionRule fallback;
        fallback.ruleId = "FALLBACK-GENERIC";
        fallback.ruleName = "通用分解";
        fallback.intentAction = requirement.intent.action;
        for (const auto& tmpl : rules_[0].templateTasks) {
            AtomicTask t = tmpl;
            t.category = "implementation";
            fallback.templateTasks.push_back(t);
        }
        // 如果没有任何规则，手动构建最小集
        if (rules_.empty()) {
            AtomicTask t1;
            t1.name = "需求分析与设计";
            t1.description = "分析需求并完成详细设计文档";
            t1.category = "analysis";
            t1.estimatedHours = 4.0;
            t1.priority = TaskPriority::HIGH;
            t1.status = TaskStatus::PENDING;
            fallback.templateTasks.push_back(t1);

            AtomicTask t2;
            t2.name = "核心功能实现";
            t2.description = "实现核心业务逻辑";
            t2.category = "implementation";
            t2.estimatedHours = 16.0;
            t2.priority = TaskPriority::HIGH;
            t2.status = TaskStatus::PENDING;
            fallback.templateTasks.push_back(t2);

            AtomicTask t3;
            t3.name = "测试与验证";
            t3.description = "编写测试用例并执行验证";
            t3.category = "testing";
            t3.estimatedHours = 8.0;
            t3.priority = TaskPriority::MEDIUM;
            t3.status = TaskStatus::PENDING;
            fallback.templateTasks.push_back(t3);

            fallback.templateDependencies.push_back({0, 1});
            fallback.templateDependencies.push_back({1, 2});
        }
        std::map<std::string, std::string> idMap;
        result.graph = applyRule(fallback, requirement, idMap);
        result.appliedRules.push_back(fallback.ruleId);
        result.confidence = 0.3;
    } else {
        // Step 2: 应用最佳匹配规则
        // 取得分最高的规则
        std::sort(matches.begin(), matches.end(),
            [](const RuleMatch& a, const RuleMatch& b) { return a.score > b.score; });

        result.graph = TaskGraph();
        result.graph.graphId = generateGraphId();
        result.graph.sourceRequirementId = requirement.requirementId;

        double totalScore = 0;
        for (const auto& m : matches) totalScore += m.score;

        for (const auto& m : matches) {
            std::map<std::string, std::string> idMapping;
            TaskGraph subGraph = applyRule(*m.rule, requirement, idMapping);
            mergeGraph(result.graph, subGraph);
            result.appliedRules.push_back(m.rule->ruleId);
        }

        result.confidence = std::min(1.0, totalScore / matches.size());
        if (matches.size() > 1) {
            result.confidence = std::min(1.0, result.confidence * 0.9); // 多规则惩罚
        }
    }

    // Step 3: 添加横切关注点
    addCrossCuttingTasks(result.graph, requirement);

    // Step 4: 推断额外依赖
    inferDependencies(result.graph, requirement);

    // Step 5: 环检测
    if (result.graph.hasCycle()) {
        auto cycle = result.graph.findCycle();
        std::string cycleStr;
        for (size_t i = 0; i < cycle.size(); ++i) {
            if (i > 0) cycleStr += " → ";
            cycleStr += cycle[i];
        }
        result.warnings.push_back("Cycle detected in task graph: " + cycleStr
            + ". Removing weakest dependency.");
        // 简单修复：移除环上的第一个软依赖
        for (auto it = result.graph.dependencies.begin();
             it != result.graph.dependencies.end(); ++it) {
            if (it->type == DependencyType::SOFT) {
                result.graph.dependencies.erase(it);
                break;
            }
        }
    }

    // Step 6: 生成调度
    result.schedule = schedule(result.graph);

    return result;
}

TaskGraph TaskDecompositionEngine::decomposeToGraph(
    const RequirementParser::StandardRequirement& requirement)
{
    auto result = decompose(requirement);
    return result.graph;
}

// ---------- 调度 ----------

ExecutionSchedule TaskDecompositionEngine::schedule(const TaskGraph& graph) {
    ExecutionSchedule sched;
    sched.scheduleId = generateScheduleId();
    sched.sourceRequirementId = graph.sourceRequirementId;

    auto groups = graph.findParallelGroups();

    int maxPar = 0;
    for (size_t i = 0; i < groups.size(); ++i) {
        ScheduleStage stage;
        stage.stageNumber = static_cast<int>(i + 1);
        stage.taskIds = groups[i];
        stage.description = "第" + std::to_string(i + 1) + "阶段 - "
            + std::to_string(groups[i].size()) + "个任务可并行";

        // 阶段命名
        if (i == 0) stage.stageName = "启动阶段";
        else if (i == groups.size() - 1) stage.stageName = "收尾阶段";
        else stage.stageName = "执行阶段 " + std::to_string(i);

        sched.stages.push_back(stage);
        maxPar = std::max(maxPar, static_cast<int>(groups[i].size()));
    }

    // 计算总工时
    double totalHours = 0;
    for (const auto& kv : graph.tasks) {
        totalHours += kv.second.estimatedHours;
    }
    sched.estimatedTotalHours = totalHours;
    sched.criticalPathHours = graph.criticalPathDuration();
    sched.maxParallelism = maxPar;

    return sched;
}

// ---------- 规则管理 ----------

void TaskDecompositionEngine::addRule(const DecompositionRule& rule) {
    rules_.push_back(rule);
}

void TaskDecompositionEngine::loadDefaultRules() {
    // ========================================
    // RULE 1: create (创建/开发类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-CREATE-001";
        rule.ruleName = "创建/开发类需求分解";
        rule.intentAction = "create";

        auto makeTask = [](const std::string& name, const std::string& desc,
                           const std::string& cat, double hours, TaskPriority pri,
                           std::vector<std::string> skills) -> AtomicTask {
            AtomicTask t;
            t.name = name;
            t.description = desc;
            t.category = cat;
            t.estimatedHours = hours;
            t.priority = pri;
            t.status = TaskStatus::PENDING;
            t.requiredSkills = skills;
            return t;
        };

        rule.templateTasks.push_back(makeTask(
            "需求分析", "深入分析需求细节，产出需求规格说明书",
            "analysis", 8.0, TaskPriority::HIGH,
            {"需求分析", "业务理解"}));

        rule.templateTasks.push_back(makeTask(
            "系统设计", "完成架构设计、数据库设计、接口设计",
            "design", 16.0, TaskPriority::HIGH,
            {"架构设计", "数据库设计", "接口设计"}));

        rule.templateTasks.push_back(makeTask(
            "核心功能实现", "实现核心业务逻辑模块",
            "implementation", 32.0, TaskPriority::CRITICAL,
            {"编程", "框架"}));

        rule.templateTasks.push_back(makeTask(
            "单元测试", "编写并执行单元测试用例",
            "testing", 12.0, TaskPriority::HIGH,
            {"测试", "单元测试"}));

        rule.templateTasks.push_back(makeTask(
            "集成测试", "执行系统集成测试",
            "testing", 8.0, TaskPriority::MEDIUM,
            {"测试", "集成测试"}));

        rule.templateTasks.push_back(makeTask(
            "代码审查", "进行代码质量审查和安全审查",
            "review", 4.0, TaskPriority::MEDIUM,
            {"代码审查", "安全"}));

        rule.templateTasks.push_back(makeTask(
            "部署上线", "准备部署包并执行上线流程",
            "deployment", 6.0, TaskPriority::HIGH,
            {"运维", "部署"}));

        rule.templateTasks.push_back(makeTask(
            "文档编写", "编写用户文档和技术文档",
            "documentation", 8.0, TaskPriority::LOW,
            {"文档"}));

        // 依赖关系
        rule.templateDependencies.push_back({0, 1}); // 需求分析 → 系统设计
        rule.templateDependencies.push_back({1, 2}); // 系统设计 → 核心实现
        rule.templateDependencies.push_back({2, 3}); // 核心实现 → 单元测试
        rule.templateDependencies.push_back({2, 5}); // 核心实现 → 代码审查
        rule.templateDependencies.push_back({3, 4}); // 单元测试 → 集成测试
        rule.templateDependencies.push_back({4, 6}); // 集成测试 → 部署上线
        rule.templateDependencies.push_back({5, 6}); // 代码审查 → 部署上线
        rule.templateDependencies.push_back({2, 7}); // 核心实现 → 文档编写

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 2: query (查询/展示类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-QUERY-001";
        rule.ruleName = "查询/展示类需求分解";
        rule.intentAction = "query";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("数据建模", "定义查询所需的数据模型和索引策略",
            "design", 8.0, TaskPriority::HIGH, {"数据建模", "SQL"}));
        rule.templateTasks.push_back(t("API接口设计", "设计查询API的输入输出规范",
            "design", 4.0, TaskPriority::HIGH, {"API设计"}));
        rule.templateTasks.push_back(t("查询逻辑实现", "实现查询业务逻辑和数据访问层",
            "implementation", 16.0, TaskPriority::CRITICAL, {"编程", "数据库"}));
        rule.templateTasks.push_back(t("性能优化", "优化查询性能，包括缓存和索引调优",
            "implementation", 8.0, TaskPriority::HIGH, {"性能优化", "缓存"}));
        rule.templateTasks.push_back(t("UI展示开发", "开发前端查询界面和数据可视化",
            "implementation", 12.0, TaskPriority::MEDIUM, {"前端", "可视化"}));
        rule.templateTasks.push_back(t("功能测试", "验证查询准确性和性能指标",
            "testing", 8.0, TaskPriority::MEDIUM, {"测试"}));

        rule.templateDependencies.push_back({0, 2});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({2, 4});
        rule.templateDependencies.push_back({3, 5});
        rule.templateDependencies.push_back({4, 5});

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 3: update (修改/更新类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-UPDATE-001";
        rule.ruleName = "修改/更新类需求分解";
        rule.intentAction = "update";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("影响评估", "评估修改对现有系统的影响范围",
            "analysis", 4.0, TaskPriority::HIGH, {"系统分析"}));
        rule.templateTasks.push_back(t("修改方案设计", "制定具体的修改技术方案",
            "design", 6.0, TaskPriority::HIGH, {"设计"}));
        rule.templateTasks.push_back(t("代码修改实现", "实施代码修改",
            "implementation", 12.0, TaskPriority::CRITICAL, {"编程"}));
        rule.templateTasks.push_back(t("回归测试", "执行回归测试确保未破坏已有功能",
            "testing", 8.0, TaskPriority::HIGH, {"测试"}));
        rule.templateTasks.push_back(t("部署验证", "部署修改并验证线上效果",
            "deployment", 4.0, TaskPriority::MEDIUM, {"运维"}));

        rule.templateDependencies.push_back({0, 1});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({3, 4});

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 4: delete (删除/清理类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-DELETE-001";
        rule.ruleName = "删除/清理类需求分解";
        rule.intentAction = "delete";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("数据依赖分析", "分析待删除数据的依赖关系和影响",
            "analysis", 4.0, TaskPriority::HIGH, {"数据分析"}));
        rule.templateTasks.push_back(t("安全审查", "审查删除操作的安全性，确保合规",
            "review", 2.0, TaskPriority::CRITICAL, {"安全", "合规"}));
        rule.templateTasks.push_back(t("删除逻辑实现", "实现数据删除/清理逻辑",
            "implementation", 8.0, TaskPriority::CRITICAL, {"编程"}));
        rule.templateTasks.push_back(t("备份与回滚", "实施删除前数据备份和回滚方案",
            "implementation", 4.0, TaskPriority::HIGH, {"数据库", "运维"}));
        rule.templateTasks.push_back(t("功能验证", "验证删除功能正确性和数据完整性",
            "testing", 6.0, TaskPriority::HIGH, {"测试"}));

        rule.templateDependencies.push_back({0, 1});
        rule.templateDependencies.push_back({0, 2});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({3, 4});

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 5: analyze (分析/统计类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-ANALYZE-001";
        rule.ruleName = "分析/统计类需求分解";
        rule.intentAction = "analyze";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("数据采集方案", "确定数据来源、采集方式和频率",
            "analysis", 6.0, TaskPriority::HIGH, {"数据分析", "ETL"}));
        rule.templateTasks.push_back(t("数据处理管道", "构建数据清洗、转换、聚合管道",
            "implementation", 16.0, TaskPriority::CRITICAL, {"数据处理", "编程"}));
        rule.templateTasks.push_back(t("算法/模型开发", "开发分析算法或统计模型",
            "implementation", 20.0, TaskPriority::CRITICAL, {"算法", "机器学习"}));
        rule.templateTasks.push_back(t("可视化报表", "开发可视化报表和仪表盘",
            "implementation", 12.0, TaskPriority::MEDIUM, {"前端", "可视化"}));
        rule.templateTasks.push_back(t("准确性验证", "验证分析结果的准确性",
            "testing", 8.0, TaskPriority::HIGH, {"测试", "数据分析"}));
        rule.templateTasks.push_back(t("性能调优", "优化大数据量下的分析性能",
            "implementation", 8.0, TaskPriority::MEDIUM, {"性能优化"}));

        rule.templateDependencies.push_back({0, 1});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({2, 4});
        rule.templateDependencies.push_back({4, 5});

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 6: configure (配置/设置类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-CONFIGURE-001";
        rule.ruleName = "配置/设置类需求分解";
        rule.intentAction = "configure";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("配置需求梳理", "梳理需要配置的项及其约束",
            "analysis", 4.0, TaskPriority::HIGH, {"系统分析"}));
        rule.templateTasks.push_back(t("配置界面/API设计", "设计配置管理的交互界面或API",
            "design", 6.0, TaskPriority::HIGH, {"设计"}));
        rule.templateTasks.push_back(t("配置功能实现", "实现配置管理核心逻辑",
            "implementation", 12.0, TaskPriority::CRITICAL, {"编程"}));
        rule.templateTasks.push_back(t("配置校验", "实现配置合法性校验和默认值处理",
            "implementation", 6.0, TaskPriority::HIGH, {"编程"}));
        rule.templateTasks.push_back(t("配置测试", "测试各种配置组合和边界情况",
            "testing", 6.0, TaskPriority::MEDIUM, {"测试"}));
        rule.templateTasks.push_back(t("配置文档", "编写配置说明和运维文档",
            "documentation", 4.0, TaskPriority::LOW, {"文档"}));

        rule.templateDependencies.push_back({0, 1});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({3, 4});
        rule.templateDependencies.push_back({2, 5});

        rules_.push_back(rule);
    }

    // ========================================
    // RULE 7: integrate (集成/对接类)
    // ========================================
    {
        DecompositionRule rule;
        rule.ruleId = "RULE-INTEGRATE-001";
        rule.ruleName = "集成/对接类需求分解";
        rule.intentAction = "integrate";

        auto t = [](const std::string& n, const std::string& d, const std::string& c,
                    double h, TaskPriority p, std::vector<std::string> s) -> AtomicTask {
            AtomicTask t; t.name=n; t.description=d; t.category=c;
            t.estimatedHours=h; t.priority=p; t.status=TaskStatus::PENDING;
            t.requiredSkills=s; return t;
        };

        rule.templateTasks.push_back(t("接口调研", "调研第三方接口文档、认证方式、限制条件",
            "analysis", 6.0, TaskPriority::HIGH, {"调研", "API"}));
        rule.templateTasks.push_back(t("适配方案设计", "设计数据转换和适配方案",
            "design", 8.0, TaskPriority::HIGH, {"架构设计"}));
        rule.templateTasks.push_back(t("适配器开发", "开发接口适配器和数据转换层",
            "implementation", 20.0, TaskPriority::CRITICAL, {"编程", "API"}));
        rule.templateTasks.push_back(t("异常处理", "实现超时重试、降级、熔断等容错机制",
            "implementation", 8.0, TaskPriority::HIGH, {"容错"}));
        rule.templateTasks.push_back(t("集成测试", "与第三方进行联调测试",
            "testing", 12.0, TaskPriority::CRITICAL, {"测试", "联调"}));
        rule.templateTasks.push_back(t("监控告警", "配置集成链路的监控和告警",
            "deployment", 4.0, TaskPriority::MEDIUM, {"运维", "监控"}));

        rule.templateDependencies.push_back({0, 1});
        rule.templateDependencies.push_back({1, 2});
        rule.templateDependencies.push_back({2, 3});
        rule.templateDependencies.push_back({3, 4});
        rule.templateDependencies.push_back({4, 5});

        rules_.push_back(rule);
    }
}

// ---------- 规则匹配 ----------

std::vector<TaskDecompositionEngine::RuleMatch>
TaskDecompositionEngine::matchRules(
    const RequirementParser::StandardRequirement& requirement) const
{
    std::vector<RuleMatch> result;

    for (const auto& rule : rules_) {
        // 按意图动作匹配
        if (!rule.intentAction.empty() && rule.intentAction != requirement.intent.action) {
            continue;
        }

        // 按实体类型匹配（检查是否存在对应类型的实体）
        if (!rule.entityType.empty()) {
            bool found = false;
            for (const auto& ent : requirement.entities) {
                if (ent.type == rule.entityType) { found = true; break; }
            }
            if (!found) continue;
        }

        // 计算匹配得分
        double score = 0.5; // 基础分

        // 意图完全匹配 +0.3
        if (!rule.intentAction.empty() && rule.intentAction == requirement.intent.action) {
            score += 0.3;
        }
        // 置信度加成
        score += requirement.intent.confidence * 0.2;

        RuleMatch match;
        match.rule = &rule;
        match.score = std::min(1.0, score);
        result.push_back(match);
    }

    return result;
}

// ---------- 规则应用 ----------

TaskGraph TaskDecompositionEngine::applyRule(
    const DecompositionRule& rule,
    const RequirementParser::StandardRequirement& requirement,
    std::map<std::string, std::string>& idMapping)
{
    TaskGraph graph;
    graph.graphId = generateGraphId();
    graph.sourceRequirementId = requirement.requirementId;

    // 复制模板任务，替换占位符
    for (size_t i = 0; i < rule.templateTasks.size(); ++i) {
        AtomicTask task = rule.templateTasks[i];
        task.taskId = generateTaskId();
        task.status = TaskStatus::PENDING;

        // 用需求信息丰富任务描述
        if (!requirement.intent.target.empty()) {
            task.tags.push_back(requirement.intent.target);
        }
        if (!requirement.summary.empty()) {
            task.tags.push_back(requirement.summary.substr(0, 30));
        }

        // 根据约束调整工时
        task.estimatedHours = estimateEffort(requirement, task);
        task.priority = inferPriority(task, requirement);

        // 记录ID映射
        idMapping[std::to_string(i)] = task.taskId;
        graph.addTask(task);
    }

    // 添加依赖
    for (const auto& depPair : rule.templateDependencies) {
        std::string fromIdx = std::to_string(depPair.first);
        std::string toIdx = std::to_string(depPair.second);
        if (idMapping.count(fromIdx) && idMapping.count(toIdx)) {
            graph.addDependency(idMapping[fromIdx], idMapping[toIdx],
                DependencyType::HARD,
                "来自规则 " + rule.ruleId + " 的模板依赖");
        }
    }

    return graph;
}

// ---------- 图合并 ----------

void TaskDecompositionEngine::mergeGraph(TaskGraph& target, const TaskGraph& source) {
    // 合并任务（不覆盖同名任务）
    for (const auto& kv : source.tasks) {
        if (!target.hasTask(kv.first)) {
            target.addTask(kv.second);
        }
    }
    // 合并依赖（去重）
    std::set<std::pair<std::string, std::string>> existingDeps;
    for (const auto& d : target.dependencies) {
        existingDeps.insert({d.fromTaskId, d.toTaskId});
    }
    for (const auto& d : source.dependencies) {
        if (existingDeps.find({d.fromTaskId, d.toTaskId}) == existingDeps.end()) {
            target.addDependency(d.fromTaskId, d.toTaskId, d.type, d.reason);
            existingDeps.insert({d.fromTaskId, d.toTaskId});
        }
    }
}

// ---------- 横切关注点 ----------

void TaskDecompositionEngine::addCrossCuttingTasks(
    TaskGraph& graph, const RequirementParser::StandardRequirement& req)
{
    // 安全检查
    bool hasSecurity = false;
    for (const auto& c : req.constraints) {
        if (c.type == "security") { hasSecurity = true; break; }
    }
    if (hasSecurity) {
        AtomicTask secReview;
        secReview.taskId = generateTaskId();
        secReview.name = "安全审查";
        secReview.description = "对需求进行安全合规审查（基于约束：" +
            req.rawInput.substr(0, 40) + "...）";
        secReview.category = "review";
        secReview.estimatedHours = 4.0;
        secReview.priority = TaskPriority::CRITICAL;
        secReview.status = TaskStatus::PENDING;
        secReview.requiredSkills = {"安全", "合规"};
        secReview.tags.push_back("security");
        graph.addTask(secReview);

        // 安全审查应该在实现完成后、部署前执行
        auto leaves = graph.getLeafTasks();
        for (const auto& leaf : leaves) {
            graph.addDependency(leaf, secReview.taskId, DependencyType::SOFT,
                "安全约束要求审查");
        }
    }

    // 性能约束
    bool hasPerformance = false;
    for (const auto& c : req.constraints) {
        if (c.type == "performance" || c.type == "scale") {
            hasPerformance = true; break;
        }
    }
    if (hasPerformance) {
        AtomicTask perfTest;
        perfTest.taskId = generateTaskId();
        perfTest.name = "性能压测";
        perfTest.description = "进行性能压力和负载测试，验证满足约束指标";
        perfTest.category = "testing";
        perfTest.estimatedHours = 8.0;
        perfTest.priority = TaskPriority::HIGH;
        perfTest.status = TaskStatus::PENDING;
        perfTest.requiredSkills = {"性能测试", "JMeter"};
        perfTest.tags.push_back("performance");
        graph.addTask(perfTest);

        auto leaves = graph.getLeafTasks();
        for (const auto& leaf : leaves) {
            graph.addDependency(leaf, perfTest.taskId, DependencyType::SOFT,
                "性能约束要求验证");
        }
    }

    // 兼容性约束
    bool hasCompat = false;
    for (const auto& c : req.constraints) {
        if (c.type == "compatibility") { hasCompat = true; break; }
    }
    if (hasCompat) {
        AtomicTask compatTest;
        compatTest.taskId = generateTaskId();
        compatTest.name = "兼容性测试";
        compatTest.description = "验证在不同平台/浏览器/环境下的兼容性";
        compatTest.category = "testing";
        compatTest.estimatedHours = 6.0;
        compatTest.priority = TaskPriority::HIGH;
        compatTest.status = TaskStatus::PENDING;
        compatTest.requiredSkills = {"测试", "多平台"};
        compatTest.tags.push_back("compatibility");
        graph.addTask(compatTest);

        auto leaves = graph.getLeafTasks();
        for (const auto& leaf : leaves) {
            graph.addDependency(leaf, compatTest.taskId, DependencyType::SOFT,
                "兼容性约束要求验证");
        }
    }
}

// ---------- 依赖推断 ----------

void TaskDecompositionEngine::inferDependencies(
    TaskGraph& graph, const RequirementParser::StandardRequirement& req)
{
    (void)req; // 未来可基于需求内容做更智能的推断

    // 基本推断规则：
    // 1. analysis → design → implementation → testing → deployment 的隐式依赖
    // 2. 同category任务之间的排序
    // 3. 基于标签的依赖推断

    // 简单实现：确保所有非根任务至少有一个前驱依赖
    auto roots = graph.getRootTasks();

    // 如果有多个根任务，尝试将它们按category排序连接
    if (roots.size() > 1) {
        // 按category优先级排序
        std::map<std::string, int> catPriority = {
            {"analysis", 0}, {"design", 1}, {"implementation", 2},
            {"testing", 3}, {"deployment", 4}, {"review", 5},
            {"documentation", 6}, {"integration", 7}
        };

        std::sort(roots.begin(), roots.end(), [&](const std::string& a, const std::string& b) {
            int pa = catPriority[graph.getTask(a).category];
            int pb = catPriority[graph.getTask(b).category];
            return pa < pb;
        });

        // 按顺序添加软依赖
        for (size_t i = 1; i < roots.size(); ++i) {
            // 检查是否已存在依赖
            bool alreadyDependent = false;
            for (const auto& dep : graph.dependencies) {
                if (dep.toTaskId == roots[i]) { alreadyDependent = true; break; }
            }
            if (!alreadyDependent) {
                graph.addDependency(roots[i - 1], roots[i], DependencyType::SOFT,
                    "推断：按类别排序的软依赖");
            }
        }
    }
}

// ---------- 工时估算 ----------

double TaskDecompositionEngine::estimateEffort(
    const RequirementParser::StandardRequirement& req,
    const AtomicTask& taskTemplate) const
{
    double base = taskTemplate.estimatedHours;
    double multiplier = getConstraintMultiplier(req) * effortMultiplier_;

    // 根据优先级微调
    switch (taskTemplate.priority) {
        case TaskPriority::CRITICAL: multiplier *= 1.0; break;
        case TaskPriority::HIGH:     multiplier *= 0.95; break;
        case TaskPriority::MEDIUM:   multiplier *= 0.9; break;
        case TaskPriority::LOW:      multiplier *= 0.85; break;
        case TaskPriority::OPTIONAL: multiplier *= 0.7; break;
    }

    return std::max(0.5, std::round(base * multiplier * 2) / 2.0); // 量化到0.5h
}

double TaskDecompositionEngine::getConstraintMultiplier(
    const RequirementParser::StandardRequirement& req) const
{
    double mult = 1.0;

    for (const auto& c : req.constraints) {
        if (c.type == "performance") mult += 0.15;   // 性能要求增加工时
        if (c.type == "security")    mult += 0.1;     // 安全要求增加工时
        if (c.type == "compatibility") mult += 0.1;   // 兼容性要求增加工时
        if (c.type == "scale")      mult += 0.2;      // 规模要求大幅增加工时
        if (c.type == "quality")    mult += 0.1;      // 质量要求增加工时
        if (c.type == "time")       mult += 0.05;     // 时间紧迫略微增加风险工时
    }

    return mult;
}

// ---------- 优先级推断 ----------

TaskPriority TaskDecompositionEngine::inferPriority(
    const AtomicTask& task,
    const RequirementParser::StandardRequirement& req) const
{
    // 关键类别任务保持高优先级
    if (task.category == "implementation" || task.category == "testing") {
        // 检查是否有性能/安全约束，提升优先级
        for (const auto& c : req.constraints) {
            if (c.type == "security" || c.type == "performance") {
                return TaskPriority::CRITICAL;
            }
        }
        return TaskPriority::HIGH;
    }
    if (task.category == "analysis" || task.category == "design") {
        return TaskPriority::HIGH;
    }
    if (task.category == "deployment") {
        return TaskPriority::MEDIUM;
    }
    if (task.category == "documentation") {
        return TaskPriority::LOW;
    }
    return task.priority; // 保持模板原有优先级
}

} // namespace TaskDecomposition
