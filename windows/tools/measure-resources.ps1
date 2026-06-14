param(
    [string]$ProcessName = "Restime"
)

$p = Get-Process -Name $ProcessName -ErrorAction Stop
[pscustomobject]@{
    Name = $p.ProcessName
    Id = $p.Id
    PrivateMB = [math]::Round($p.PrivateMemorySize64 / 1MB, 2)
    WorkingSetMB = [math]::Round($p.WorkingSet64 / 1MB, 2)
    Threads = $p.Threads.Count
    Handles = $p.HandleCount
    CPUSeconds = $p.CPU
}
