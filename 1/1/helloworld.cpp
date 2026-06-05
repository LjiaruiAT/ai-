#include "RequirementParser.hpp"
#include <iostream>
#include <iomanip>

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

void analyzeRequirement(const std::string& rawInput) {
    RequirementParser::Parser parser;
    auto req = parser.parse(rawInput);

    std::cout << "\n[原始输入] " << rawInput << "\n\n";
    std::cout << "[解析结果]\n";
    std::cout << req.toPrettyJSON() << "\n";
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        需求解析模块 (Requirement Parsing Module)            ║\n";
    std::cout << "║        自然语言需求 → 结构化 JSON 描述                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    // ==================== 测试用例 ====================

    printSeparator("案例1：创建类需求");
    analyzeRequirement(
        "管理员需要创建一个用户管理模块，"
        "支持新增、修改、删除用户，"
        "在3周内完成开发，预算不超过5万元。"
    );

    printSeparator("案例2：查询类需求");
    analyzeRequirement(
        "客户希望能够查询订单的实时状态，"
        "响应时间不超过200ms，"
        "支持1000个并发用户同时查询。"
    );

    printSeparator("案例3：集成类需求");
    analyzeRequirement(
        "需要将ERP系统与WMS系统进行对接，"
        "实现库存数据的实时同步，"
        "兼容Windows和Linux平台，"
        "数据传输需要加密保证安全性。"
    );

    printSeparator("案例4：分析类需求");
    analyzeRequirement(
        "运营经理需要一个销售数据统计报表，"
        "分析每月的销售趋势和客户增长，"
        "准确率不低于99.9%，"
        "每天凌晨自动生成。"
    );

    printSeparator("案例5：配置类需求");
    analyzeRequirement(
        "系统管理员需要配置权限管理模块，"
        "设置不同角色的访问控制策略，"
        "支持SSO单点登录和OAuth认证，"
        "配置完成后需要审计日志记录所有操作。"
    );

    printSeparator("案例6：带多个约束的复杂需求");
    analyzeRequirement(
        "开发一个在线支付功能模块，"
        "用户可以通过支付宝和微信支付，"
        "响应时间不超过500毫秒，"
        "支持5000并发用户，"
        "数据需要HTTPS加密传输，"
        "必须在本月底之前上线，"
        "预算控制在10万元以内，"
        "可用性达到99.99%。"
    );

    // ==================== 交互模式 ====================
    printSeparator("交互模式");
    std::cout << "\n输入自然语言需求（输入 exit 退出）：\n\n";

    RequirementParser::Parser parser;
    std::string input;
    while (true) {
        std::cout << ">>> ";
        std::getline(std::cin, input);
        if (input == "exit" || input == "quit" || input == "退出") {
            std::cout << "再见！\n";
            break;
        }
        if (input.empty()) continue;

        auto req = parser.parse(input);
        std::cout << "\n" << req.toPrettyJSON() << "\n\n";
    }

    return 0;
}
