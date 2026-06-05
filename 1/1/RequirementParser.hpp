#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace RequirementParser {

// ==================== 基础数据结构 ====================

/** 关键实体 */
struct Entity {
    std::string name;       // 实体名称
    std::string type;       // 类型：person, system, module, data, time, location, metric
    std::string role;       // 角色：actor, target, context

    std::string toJSON() const;
};

/** 意图 */
struct Intent {
    std::string action;      // 动作类型：create, query, update, delete, analyze, configure, integrate
    std::string target;      // 动作目标
    std::string description; // 原始描述片段
    double confidence;       // 置信度 0.0 ~ 1.0

    std::string toJSON() const;
};

/** 约束条件 */
struct Constraint {
    std::string type;        // 类型：time, performance, budget, compatibility, security, scale, quality
    std::string field;       // 字段名
    std::string operator_;   // 操作符：eq, lt, gt, lte, gte, range, contains
    std::string value;       // 约束值
    std::string description; // 原始描述

    std::string toJSON() const;
};

/** 标准化需求描述 */
struct StandardRequirement {
    std::string requirementId;
    std::string rawInput;
    std::vector<Entity> entities;
    Intent intent;
    std::vector<Constraint> constraints;
    std::string summary;
    std::string timestamp;

    std::string toJSON() const;
    std::string toPrettyJSON() const;
};

// ==================== 解析器 ====================

class Parser {
public:
    Parser();

    /**
     * 解析自然语言需求，返回结构化需求描述
     * @param rawInput 用户原始自然语言输入
     * @return StandardRequirement 标准化需求描述
     */
    StandardRequirement parse(const std::string& rawInput);

    /**
     * 解析并直接返回 JSON 字符串
     */
    std::string parseToJSON(const std::string& rawInput, bool pretty = false);

private:
    int requirementCounter_;

    // ---------- 提取子模块 ----------
    std::vector<Entity> extractEntities(const std::string& input);
    Intent extractIntent(const std::string& input);
    std::vector<Constraint> extractConstraints(const std::string& input);
    std::string generateSummary(const StandardRequirement& req);

    // ---------- 辅助方法 ----------
    std::string generateId();
    std::string getTimestamp() const;

    // 中文分词辅助（简易基于词典）
    std::vector<std::string> tokenize(const std::string& input) const;
    bool containsAny(const std::string& text, const std::vector<std::string>& keywords) const;
    std::string extractAfter(const std::string& text, const std::string& keyword) const;
    std::string extractBetween(const std::string& text,
                               const std::string& left,
                               const std::string& right) const;

    // 实体词典
    std::map<std::string, std::string> entityDictionary_;   // name -> type
    std::vector<std::string> personKeywords_;
    std::vector<std::string> systemKeywords_;
    std::vector<std::string> moduleKeywords_;
    std::vector<std::string> timeKeywords_;
    std::vector<std::string> locationKeywords_;

    // 意图词典
    std::map<std::string, std::string> intentActionMap_;    // keyword -> action
    std::vector<std::string> createKeywords_;
    std::vector<std::string> queryKeywords_;
    std::vector<std::string> updateKeywords_;
    std::vector<std::string> deleteKeywords_;
    std::vector<std::string> analyzeKeywords_;
    std::vector<std::string> configureKeywords_;
    std::vector<std::string> integrateKeywords_;

    // 约束词典
    std::vector<std::string> timeConstraintKeywords_;
    std::vector<std::string> performanceKeywords_;
    std::vector<std::string> budgetKeywords_;
    std::vector<std::string> compatibilityKeywords_;
    std::vector<std::string> securityKeywords_;
    std::vector<std::string> scaleKeywords_;
    std::vector<std::string> qualityKeywords_;
};

} // namespace RequirementParser
