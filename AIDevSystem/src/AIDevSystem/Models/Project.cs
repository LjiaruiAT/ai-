namespace AIDevSystem.Models;

public class Project
{
    public int Id { get; set; }
    public string Name { get; set; } = "";
    public string? Description { get; set; }
    public string? RootPath { get; set; }
    public string? CreatedAt { get; set; }
    public string? UpdatedAt { get; set; }
}
