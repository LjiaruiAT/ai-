#include "RequirementParser.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <set>

namespace RequirementParser {

// ==================== JSON 序列化辅助 ====================

namespace {
    std::string jsonEscape(const std::string& s) {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;
            }
        }
        return out;
    }
}

// ==================== Entity::toJSON ====================

std::string Entity::toJSON() const {
    std::ostringstream oss;
    oss << "{\"name\":\"" << jsonEscape(name)
        << "\",\"type\":\"" << jsonEscape(type)
        << "\",\"role\":\"" << jsonEscape(role) << "\"}";
    return oss.str();
}

// ==================== Intent::toJSON ====================

std::string Intent::toJSON() const {
    std::ostringstream oss;
    oss << "{\"action\":\"" << jsonEscape(action)
        << "\",\"target\":\"" << jsonEscape(target)
        << "\",\"description\":\"" << jsonEscape(description)
        << "\",\"confidence\":" << confidence << "}";
    return oss.str();
}

// ==================== Constraint::toJSON ====================

std::string Constraint::toJSON() const {
    std::ostringstream oss;
    oss << "{\"type\":\"" << jsonEscape(type)
        << "\",\"field\":\"" << jsonEscape(field)
        << "\",\"operator\":\"" << jsonEscape(operator_)
        << "\",\"value\":\"" << jsonEscape(value)
        << "\",\"description\":\"" << jsonEscape(description) << "\"}";
    return oss.str();
}

// ==================== StandardRequirement::toJSON ====================

std::string StandardRequirement::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"requirementId\":\"" << jsonEscape(requirementId) << "\","
        << "\"rawInput\":\"" << jsonEscape(rawInput) << "\","
        << "\"entities\":[";
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) oss << ",";
        oss << entities[i].toJSON();
    }
    oss << "],\"intent\":" << intent.toJSON()
        << ",\"constraints\":[";
    for (size_t i = 0; i < constraints.size(); ++i) {
        if (i > 0) oss << ",";
        oss << constraints[i].toJSON();
    }
    oss << "],\"summary\":\"" << jsonEscape(summary)
        << "\",\"timestamp\":\"" << jsonEscape(timestamp) << "\"}";
    return oss.str();
}

std::string StandardRequirement::toPrettyJSON() const {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"requirementId\": \"" << jsonEscape(requirementId) << "\",\n"
        << "  \"rawInput\": \"" << jsonEscape(rawInput) << "\",\n"
        << "  \"entities\": [\n";
    for (size_t i = 0; i < entities.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << "    " << entities[i].toJSON();
    }
    oss << "\n  ],\n"
        << "  \"intent\": " << intent.toJSON() << ",\n"
        << "  \"constraints\": [\n";
    for (size_t i = 0; i < constraints.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << "    " << constraints[i].toJSON();
    }
    oss << "\n  ],\n"
        << "  \"summary\": \"" << jsonEscape(summary) << "\",\n"
        << "  \"timestamp\": \"" << jsonEscape(timestamp) << "\"\n"
        << "}";
    return oss.str();
}

// ==================== Parser 实现 ====================

Parser::Parser()
    : requirementCounter_(0)
{
    // ---------- 实体词典 ----------
    entityDictionary_["用户"] = "person";
    entityDictionary_["管理员"] = "person";
    entityDictionary_["客户"] = "person";
    entityDictionary_["员工"] = "person";
    entityDictionary_["开发人员"] = "person";
    entityDictionary_["运维人员"] = "person";
    entityDictionary_["访客"] = "person";
    entityDictionary_["系统"] = "system";
    entityDictionary_["后台"] = "system";
    entityDictionary_["前端"] = "system";
    entityDictionary_["后端"] = "system";
    entityDictionary_["服务"] = "system";
    entityDictionary_["平台"] = "system";
    entityDictionary_["模块"] = "module";
    entityDictionary_["组件"] = "module";
    entityDictionary_["功能"] = "module";
    entityDictionary_["接口"] = "module";
    entityDictionary_["API"] = "module";
    entityDictionary_["数据库"] = "data";
    entityDictionary_["数据"] = "data";
    entityDictionary_["表"] = "data";
    entityDictionary_["日志"] = "data";
    entityDictionary_["配置"] = "data";
    entityDictionary_["报表"] = "data";

    // 人物关键词
    personKeywords_ = {"用户", "管理员", "客户", "员工", "人员", "访客", "开发者", "运维"};

    // 系统关键词
    systemKeywords_ = {"系统", "平台", "服务", "后台", "后端", "前端", "服务器", "应用"};

    // 模块关键词
    moduleKeywords_ = {"模块", "组件", "功能", "接口", "API", "插件", "库"};

    // 时间关键词
    timeKeywords_ = {"毫秒", "秒", "分钟", "小时", "天", "周", "月", "年",
                     "实时", "立即", "快速", "延时", "定时", "周期", "截止"};

    // 地点关键词
    locationKeywords_ = {"浏览器", "客户端", "移动端", "桌面", "云端", "本地", "远程",
                         "服务器", "容器", "集群"};

    // ---------- 意图词典 ----------
    intentActionMap_["创建"] = "create";
    intentActionMap_["新建"] = "create";
    intentActionMap_["开发"] = "create";
    intentActionMap_["搭建"] = "create";
    intentActionMap_["构建"] = "create";
    intentActionMap_["实现"] = "create";
    intentActionMap_["编写"] = "create";

    intentActionMap_["查询"] = "query";
    intentActionMap_["搜索"] = "query";
    intentActionMap_["检索"] = "query";
    intentActionMap_["查看"] = "query";
    intentActionMap_["展示"] = "query";
    intentActionMap_["显示"] = "query";
    intentActionMap_["统计"] = "query";
    intentActionMap_["导出"] = "query";
    intentActionMap_["生成报表"] = "query";

    intentActionMap_["修改"] = "update";
    intentActionMap_["更新"] = "update";
    intentActionMap_["优化"] = "update";
    intentActionMap_["重构"] = "update";
    intentActionMap_["改进"] = "update";
    intentActionMap_["升级"] = "update";
    intentActionMap_["调整"] = "update";
    intentActionMap_["变更"] = "update";

    intentActionMap_["删除"] = "delete";
    intentActionMap_["移除"] = "delete";
    intentActionMap_["废弃"] = "delete";
    intentActionMap_["下线"] = "delete";
    intentActionMap_["清理"] = "delete";
    intentActionMap_["注销"] = "delete";

    intentActionMap_["分析"] = "analyze";
    intentActionMap_["评估"] = "analyze";
    intentActionMap_["诊断"] = "analyze";
    intentActionMap_["审查"] = "analyze";
    intentActionMap_["研究"] = "analyze";
    intentActionMap_["调研"] = "analyze";
    intentActionMap_["监控"] = "analyze";
    intentActionMap_["审计"] = "analyze";

    intentActionMap_["配置"] = "configure";
    intentActionMap_["设置"] = "configure";
    intentActionMap_["安装"] = "configure";
    intentActionMap_["部署"] = "configure";
    intentActionMap_["初始化"] = "configure";
    intentActionMap_["启动"] = "configure";
    intentActionMap_["接入"] = "configure";

    intentActionMap_["集成"] = "integrate";
    intentActionMap_["对接"] = "integrate";
    intentActionMap_["连接"] = "integrate";
    intentActionMap_["打通"] = "integrate";
    intentActionMap_["同步"] = "integrate";
    intentActionMap_["迁移"] = "integrate";
    intentActionMap_["导入"] = "integrate";

    // 动作分类关键词（用于快速匹配）
    createKeywords_    = {"创建","新建","开发","搭建","构建","实现","编写","设计","制作","建立"};
    queryKeywords_     = {"查询","搜索","检索","查看","展示","显示","统计","导出","获取","读取"};
    updateKeywords_    = {"修改","更新","优化","重构","改进","升级","调整","变更","修复","增强"};
    deleteKeywords_    = {"删除","移除","废弃","下线","清理","注销","销毁","停用"};
    analyzeKeywords_   = {"分析","评估","诊断","审查","研究","调研","监控","审计","检测","检查"};
    configureKeywords_ = {"配置","设置","安装","部署","初始化","启动","搭建环境","装配"};
    integrateKeywords_ = {"集成","对接","连接","打通","同步","迁移","导入","合并","接入"};

    // ---------- 约束词典 ----------
    timeConstraintKeywords_    = {"毫秒","秒","分钟","小时","天","周","月","截止","期限","时间",
                                  "延迟","响应","实时","速度","快速","效率"};
    performanceKeywords_       = {"QPS","TPS","吞吐","并发","性能","响应时间","延迟","负载",
                                  "容量","压力"};
    budgetKeywords_            = {"预算","成本","费用","人力","工期","资源","工作量"};
    compatibilityKeywords_     = {"兼容","适配","支持","浏览器","平台","版本","系统","环境",
                                  "操作系统"};
    securityKeywords_          = {"安全","加密","权限","认证","鉴权","审计","脱敏","合规",
                                  "SQL注入","XSS","CSRF","漏洞","HTTPS","TLS"};
    scaleKeywords_             = {"规模","用户量","数据量","并发数","集群","分布式","扩展",
                                  "弹性","容量"};
    qualityKeywords_           = {"质量","可靠","可用","稳定","容错","鲁棒","SLA","准确",
                                  "精确","一致","完整"};
}

// ---------- ID 和时间戳 ----------

std::string Parser::generateId() {
    std::ostringstream oss;
    oss << "REQ-" << std::setw(4) << std::setfill('0') << (++requirementCounter_);
    return oss.str();
}

std::string Parser::getTimestamp() const {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

// ---------- 中文分词（简易） ----------

std::vector<std::string> Parser::tokenize(const std::string& input) const {
    std::vector<std::string> tokens;
    std::string current;
    // 按标点和大词切分
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        // 标点作为分隔
        if (c == '，' || c == ',' || c == '。' || c == '.' ||
            c == '；' || c == ';' || c == '！' || c == '!' ||
            c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current += c;

        // 检查是否为已知词汇（最大匹配）
        for (const auto& kv : entityDictionary_) {
            if (current.size() >= kv.first.size() &&
                current.substr(current.size() - kv.first.size()) == kv.first) {
                // 已匹配，继续累积
                break;
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

bool Parser::containsAny(const std::string& text,
                         const std::vector<std::string>& keywords) const {
    for (const auto& kw : keywords) {
        if (text.find(kw) != std::string::npos) return true;
    }
    return false;
}

std::string Parser::extractAfter(const std::string& text,
                                  const std::string& keyword) const {
    size_t pos = text.find(keyword);
    if (pos == std::string::npos) return "";
    return text.substr(pos + keyword.size());
}

std::string Parser::extractBetween(const std::string& text,
                                    const std::string& left,
                                    const std::string& right) const {
    size_t lpos = text.find(left);
    if (lpos == std::string::npos) return "";
    lpos += left.size();
    size_t rpos = text.find(right, lpos);
    if (rpos == std::string::npos) return text.substr(lpos);
    return text.substr(lpos, rpos - lpos);
}

// ---------- 实体提取 ----------

std::vector<Entity> Parser::extractEntities(const std::string& input) {
    std::vector<Entity> entities;
    std::set<std::string> seen; // 去重

    // 扫描已知实体
    for (const auto& kv : entityDictionary_) {
        if (input.find(kv.first) != std::string::npos && seen.find(kv.first) == seen.end()) {
            Entity e;
            e.name = kv.first;
            e.type = kv.second;

            // 确定角色
            if (kv.second == "person") {
                // 如果出现在"为..."、"给..."后面 → target
                if (input.find("为" + kv.first) != std::string::npos ||
                    input.find("给" + kv.first) != std::string::npos) {
                    e.role = "target";
                } else {
                    e.role = "actor";
                }
            } else if (kv.second == "system" || kv.second == "module") {
                e.role = "target";  // 系统/模块通常是被操作目标
            } else if (kv.second == "data") {
                e.role = "target";
            } else {
                e.role = "context";
            }

            entities.push_back(e);
            seen.insert(kv.first);
        }
    }

    return entities;
}

// ---------- 意图提取 ----------

Intent Parser::extractIntent(const std::string& input) {
    Intent intent;
    intent.confidence = 0.5;

    // 按优先级匹配意图关键词
    struct IntentCandidate {
        std::string action;
        std::string keyword;
        size_t position;
    };
    std::vector<IntentCandidate> candidates;

    for (const auto& kv : intentActionMap_) {
        size_t pos = input.find(kv.first);
        if (pos != std::string::npos) {
            candidates.push_back({kv.second, kv.first, pos});
        }
    }

    if (candidates.empty()) {
        intent.action = "create";   // 默认
        intent.target = input.size() > 30 ? input.substr(0, 30) + "..." : input;
        intent.description = input;
        intent.confidence = 0.3;
        return intent;
    }

    // 按位置排序（越早出现，越可能是主要意图）
    std::sort(candidates.begin(), candidates.end(),
              [](const IntentCandidate& a, const IntentCandidate& b) {
                  return a.position < b.position;
              });

    intent.action = candidates[0].action;

    // 提取目标：意图关键词后面的部分
    std::string after = extractAfter(input, candidates[0].keyword);
    if (!after.empty()) {
        // 清理标点前缀
        while (!after.empty() && (after[0] == ':' || after[0] == '：' ||
               after[0] == ' ' || after[0] == '\t')) {
            after = after.substr(1);
        }
        intent.target = after.size() > 60 ? after.substr(0, 60) + "..." : after;
    } else {
        intent.target = input;
    }

    intent.description = input;

    // 置信度：关键词越明确越高
    if (candidates.size() >= 2 && candidates[0].position < 10) {
        intent.confidence = 0.85;
    } else if (candidates[0].position < 10) {
        intent.confidence = 0.75;
    } else {
        intent.confidence = 0.6;
    }

    return intent;
}

// ---------- 约束提取 ----------

std::vector<Constraint> Parser::extractConstraints(const std::string& input) {
    std::vector<Constraint> constraints;

    // 时间约束
    for (const auto& kw : timeConstraintKeywords_) {
        size_t pos = input.find(kw);
        if (pos != std::string::npos) {
            Constraint c;
            c.type = "time";
            c.field = "latency";
            c.operator_ = "lt";
            c.value = kw;

            // 尝试提取数值
            std::string before = input.substr(0, pos);
            // 找最近数字
            std::string numStr;
            for (int i = (int)pos - 1; i >= 0; --i) {
                if (std::isdigit(input[i]) || input[i] == '.') {
                    numStr = input[i] + numStr;
                } else if (!numStr.empty()) {
                    break;
                }
            }
            if (!numStr.empty()) {
                c.value = numStr + kw;
            }

            c.description = "时间相关约束: " + c.value;
            constraints.push_back(c);
            break;  // 只取最显著的一个
        }
    }

    // 性能约束
    for (const auto& kw : performanceKeywords_) {
        if (input.find(kw) != std::string::npos) {
            Constraint c;
            c.type = "performance";
            c.field = "throughput";
            c.operator_ = "gt";
            c.value = kw;
            c.description = "性能约束: " + kw;
            constraints.push_back(c);
            break;
        }
    }

    // 安全约束
    for (const auto& kw : securityKeywords_) {
        if (input.find(kw) != std::string::npos) {
            Constraint c;
            c.type = "security";
            c.field = "security_level";
            c.operator_ = "contains";
            c.value = kw;
            c.description = "安全约束: " + kw;
            constraints.push_back(c);
            break;
        }
    }

    // 规模约束
    for (const auto& kw : scaleKeywords_) {
        if (input.find(kw) != std::string::npos) {
            Constraint c;
            c.type = "scale";
            c.field = "capacity";
            c.operator_ = "gt";
            c.value = kw;
            c.description = "规模约束: " + kw;
            constraints.push_back(c);
            break;
        }
    }

    // 质量约束
    for (const auto& kw : qualityKeywords_) {
        if (input.find(kw) != std::string::npos) {
            Constraint c;
            c.type = "quality";
            c.field = "reliability";
            c.operator_ = "contains";
            c.value = kw;
            c.description = "质量约束: " + kw;
            constraints.push_back(c);
            break;
        }
    }

    return constraints;
}

// ---------- 摘要生成 ----------

std::string Parser::generateSummary(const StandardRequirement& req) {
    std::ostringstream oss;
    oss << "需求[" << req.requirementId << "]: ";

    // 意图
    std::string actionCN;
    if (req.intent.action == "create") actionCN = "创建";
    else if (req.intent.action == "query") actionCN = "查询";
    else if (req.intent.action == "update") actionCN = "更新";
    else if (req.intent.action == "delete") actionCN = "删除";
    else if (req.intent.action == "analyze") actionCN = "分析";
    else if (req.intent.action == "configure") actionCN = "配置";
    else if (req.intent.action == "integrate") actionCN = "集成";
    else actionCN = req.intent.action;

    oss << actionCN;

    // 目标
    if (!req.intent.target.empty()) {
        oss << "「" << req.intent.target.substr(0, 40) << "」";
    }

    // 关键实体
    if (!req.entities.empty()) {
        oss << "，涉及: ";
        for (size_t i = 0; i < std::min(req.entities.size(), (size_t)3); ++i) {
            if (i > 0) oss << "、";
            oss << req.entities[i].name << "(" << req.entities[i].type << ")";
        }
    }

    // 约束
    if (!req.constraints.empty()) {
        oss << "，约束: ";
        for (size_t i = 0; i < std::min(req.constraints.size(), (size_t)2); ++i) {
            if (i > 0) oss << "、";
            oss << req.constraints[i].type;
        }
    }

    return oss.str();
}

// ---------- 主解析入口 ----------

StandardRequirement Parser::parse(const std::string& rawInput) {
    StandardRequirement req;
    req.requirementId = generateId();
    req.rawInput = rawInput;
    req.timestamp = getTimestamp();

    req.entities = extractEntities(rawInput);
    req.intent = extractIntent(rawInput);
    req.constraints = extractConstraints(rawInput);
    req.summary = generateSummary(req);

    return req;
}

std::string Parser::parseToJSON(const std::string& rawInput, bool pretty) {
    StandardRequirement req = parse(rawInput);
    return pretty ? req.toPrettyJSON() : req.toJSON();
}

} // namespace RequirementParser
