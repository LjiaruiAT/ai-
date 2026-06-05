namespace AIDevSystem.Models;

public class ChatMessage
{
    public int Id { get; set; }
    public int ProjectId { get; set; }
    public string Role { get; set; } = "";    // user | pm | system
    public string Content { get; set; } = "";
    public string? CreatedAt { get; set; }
}
