using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace AIDevSystem.Infrastructure;

/// <summary>
/// DeepSeek API 直连客户端。带自动重试。
/// </summary>
public class DeepSeekApiClient
{
    private readonly HttpClient _http;
    private const string ApiUrl = "https://api.deepseek.com/chat/completions";

    public DeepSeekApiClient()
    {
        _http = new HttpClient { Timeout = TimeSpan.FromMinutes(2) };
    }

    public void SetApiKey(string key)
    {
        _http.DefaultRequestHeaders.Authorization =
            new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", key);
    }

    public async Task<string> ChatAsync(string systemPrompt, string userMessage, string model = "deepseek-chat")
    {
        var body = new
        {
            model,
            messages = new[]
            {
                new { role = "system", content = systemPrompt },
                new { role = "user", content = userMessage }
            },
            temperature = 0.3,
            max_tokens = 4096
        };
        return await SendWithRetryAsync(JsonConvert.SerializeObject(body));
    }

    public async Task<string> ChatWithHistoryAsync(
        string systemPrompt,
        (string role, string text)[] history,
        string model = "deepseek-chat")
    {
        var messages = new JArray();
        messages.Add(new JObject { ["role"] = "system", ["content"] = systemPrompt });

        foreach (var (role, text) in history)
            messages.Add(new JObject { ["role"] = role, ["content"] = text });

        var body = new { model, messages, temperature = 0.3, max_tokens = 4096 };
        return await SendWithRetryAsync(JsonConvert.SerializeObject(body));
    }

    public async Task<string> ChatForJsonAsync(string systemPrompt, string userMessage, string model = "deepseek-chat")
    {
        return await ChatAsync(systemPrompt, userMessage, model);
    }

    /// 带重试 + 友好错误提示
    private async Task<string> SendWithRetryAsync(string json, int maxRetries = 3)
    {
        Exception? lastEx = null;
        for (int i = 0; i < maxRetries; i++)
        {
            try
            {
                var content = new StringContent(json, Encoding.UTF8, "application/json");
                var response = await _http.PostAsync(ApiUrl, content);
                var responseBody = await response.Content.ReadAsStringAsync();

                if (!response.IsSuccessStatusCode)
                    throw new Exception($"API 错误 ({response.StatusCode}): {responseBody}");

                var result = JObject.Parse(responseBody);
                return result["choices"]?[0]?["message"]?["content"]?.ToString()?.Trim() ?? "";
            }
            catch (HttpRequestException ex)
            {
                lastEx = ex;
                if (i < maxRetries - 1) await Task.Delay(1000 * (i + 1));
            }
            catch (TaskCanceledException)
            {
                lastEx = new Exception("请求超时，请检查网络后重试");
                if (i < maxRetries - 1) await Task.Delay(2000);
            }
            catch (Exception ex) when (!ex.Message.StartsWith("API 错误"))
            {
                lastEx = ex;
                if (i < maxRetries - 1) await Task.Delay(1000 * (i + 1));
            }
        }
        throw new Exception($"请求失败（重试{maxRetries}次后仍失败）: {lastEx?.Message}");
    }
}
