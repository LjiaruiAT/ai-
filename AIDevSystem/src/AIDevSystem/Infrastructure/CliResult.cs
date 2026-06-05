namespace AIDevSystem.Infrastructure;

//=============================================================================
// CliResult — CLI 命令执行结果
//=============================================================================

public class CliResult
{
    public int ExitCode { get; set; }
    public string Stdout { get; set; } = string.Empty;
    public string Stderr { get; set; } = string.Empty;
    public long DurationMs { get; set; }
    public bool TimedOut { get; set; }

    public bool Success => ExitCode == 0 && !TimedOut;
    public string FullOutput => string.IsNullOrEmpty(Stdout) ? Stderr : Stdout;
}
