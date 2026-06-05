namespace AIDevSystem.ViewModels;

public class HelpViewModel : BaseViewModel
{
    /// <summary>
    /// 内嵌 HTML 格式的用户指南，完全不依赖外部文件。
    /// </summary>
    public string GuideHtml { get; } = @"
<!DOCTYPE html><html><head><meta charset='UTF-8'><style>
*{margin:0;padding:0;box-sizing:border-box;}
body{font-family:'Microsoft YaHei',sans-serif;line-height:1.8;color:#e0e0e0;background:#1e1e2e;padding:20px 40px;}
h1{color:#7df9ff;font-size:1.8em;text-align:center;margin-bottom:8px;}
h2{color:#7df9ff;font-size:1.3em;border-left:3px solid #7df9ff;padding-left:10px;margin:30px 0 12px;}
h3{color:#fbbf24;font-size:1.05em;margin:20px 0 8px;}
p{margin-bottom:10px;}
.card{background:#2a2a3e;border-radius:6px;padding:20px;margin-bottom:16px;}
.steps{counter-reset:s;list-style:none;}
.steps li{counter-increment:s;margin-bottom:14px;padding-left:36px;position:relative;}
.steps li::before{content:counter(s);position:absolute;left:0;top:0;width:24px;height:24px;background:#7c3aed;color:#fff;border-radius:50%;text-align:center;line-height:24px;font-weight:bold;font-size:13px;}
table{width:100%;border-collapse:collapse;margin:12px 0;}
th,td{border:1px solid #3b3b5c;padding:8px 12px;text-align:left;font-size:13px;}
th{background:#2a2a3e;color:#7df9ff;}
.tip{background:#2d2a00;border-left:3px solid #fbbf24;padding:10px 14px;margin:12px 0;border-radius:0 4px 4px 0;}
.warn{background:#3d1a1a;border-left:3px solid #ef4444;padding:10px 14px;margin:12px 0;border-radius:0 4px 4px 0;}
.footer{text-align:center;color:#666;margin-top:40px;font-size:12px;}
.emoji{font-size:1.1em;}
code{background:#1e1e2e;padding:2px 6px;border-radius:3px;font-size:12px;}
</style></head><body>

<h1>&#x1F916; AI 开发系统 — 用户指南</h1>
<p style='text-align:center;color:#a9bad7;'>v2.0 | 作者：刘家瑞 | 河北科技大学</p>

<div class='card'>
<h2>&#x1F4CB; 一、这是什么？</h2>
<p>一个 <b>AI 驱动的软件开发系统</b>。你输入需求，AI 项目经理帮你拆成具体任务，AI 开发员工直接在你的电脑上写代码、编译、运行。</p>
<p>就像一家只有两个人的 AI 软件公司，你是老板，它们是你的员工。</p>
<table>
<tr><th>角色</th><th>做什么</th></tr>
<tr><td>&#x1F9D1;&#x200D;&#x1F4BC; 项目经理 (PM)</td><td>讨论需求、拆解任务、审查代码质量</td></tr>
<tr><td>&#x1F468;&#x200D;&#x1F4BB; 开发员工 (Dev)</td><td>写代码、编译、修复错误、打包交付</td></tr>
</table>
</div>

<div class='card'>
<h2>&#x26A1; 二、快速上手</h2>
<ol class='steps'>
<li>左侧 &#x2699; <b>设置</b> → 填入 DeepSeek API Key → 保存</li>
<li>左侧 &#x1F4C1; <b>项目</b> → 输入项目名 → 创建 → 点击选中</li>
<li>输入需求（如：<i>'在桌面创建一个 Rust HelloWorld 项目'</i>）</li>
<li>点击 &#x1F9E0; <b>拆解</b> → AI 自动生成子任务</li>
<li>点击 &#x25B6; <b>执行全部</b> → AI 开始写代码、编译、运行</li>
</ol>

<div class='tip'><b>&#x1F4A1; 推荐：</b>先点 &#x1F4AC; <b>与PM讨论</b> 把需求聊清楚再拆解，效果最好。特别是要分发给别人的程序，一定在讨论时说明。</div>
</div>

<div class='card'>
<h2>&#x1F527; 三、界面功能</h2>
<table>
<tr><th>页面</th><th>功能</th></tr>
<tr><td>&#x1F4C1; 项目</td><td>创建项目、输入需求、拆解任务、执行代码、进度条</td></tr>
<tr><td>&#x1F4AC; 讨论</td><td>与 AI 项目经理对话，确认需求细节</td></tr>
<tr><td>&#x2699; 设置</td><td>配置 API Key、选择 PM 模型、切换自动/手动模式</td></tr>
<tr><td>&#x1F4CA; 仪表盘</td><td>查看各 AI 模型的历史评分统计</td></tr>
<tr><td>&#x2753; 帮助</td><td>本指南</td></tr>
</table>

<h3>PM 审查开关</h3>
<p>项目页右侧有 <b>PM审查</b> 复选框：开启后每个任务完成后 AI 会审查代码质量，不通过自动打回重做（最多 3 次）。</p>

<h3>查看任务详情</h3>
<p>任务列表点 &#x1F4CB; → 查看执行记录和 AI 输出 → 点 <b>返回项目列表</b> 退出。</p>
</div>

<div class='card'>
<h2>&#x2753; 四、常见问题</h2>

<h3>Q: 点击拆解没反应？</h3>
<p>A: 去设置页检查 API Key 是否正确填写，确保网络正常。</p>

<h3>Q: 执行很慢？</h3>
<p>A: 开启了 PM 审查会多一次代码审查。关闭 PM 审查会快很多。每次任务约 2-5 秒。</p>

<h3>Q: AI 写的代码在哪？</h3>
<p>A: AI 会把文件写到任务描述中指定的路径。点 &#x1F4CB; 可以看 AI 的完整输出日志。</p>

<h3>Q: 发给别人跑不了？</h3>
<p>A: 和 PM 讨论时说 <b>'要分发给其他电脑'</b>，AI 会做静态编译打包所有依赖。</p>

<div class='warn'><b>&#x26A0; 注意：</b>应用需要网络连接 + DeepSeek API Key 才能工作。</div>
</div>

<div class='card'>
<h2>&#x1F4CA; 五、示例：创建贪吃蛇游戏</h2>
<ol class='steps'>
<li>点 &#x1F4AC; <b>与PM讨论</b>，输入：<b>我想做贪吃蛇游戏，C++ SDL2，要能分发给朋友</b></li>
<li>PM 会问：什么按键？要不要暂停/难度？回答清楚</li>
<li>点 &#x2705; <b>批准并拆解</b></li>
<li>回到项目页 → 勾选 PM审查 → 点 &#x25B6; <b>执行全部</b></li>
<li>等进度条走完，去项目文件夹拿结果</li>
</ol>
</div>

<div class='footer'>
<p>AI 开发系统 v2.0 | 作者：刘家瑞 | 河北科技大学</p>
<p>完全 AI 辅助开发 | 开发记录：桌面\开发记录\</p>
</div>

</body></html>";
}
