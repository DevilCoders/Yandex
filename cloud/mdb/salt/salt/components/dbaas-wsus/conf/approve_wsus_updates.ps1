param(
    [Parameter(Mandatory)]
    [int]$DaysBack,
    [Parameter(Mandatory)]
    [string]$TargetGroupName,
    [switch]$DryRun
)

$n = (Get-WsusUpdate -Classification all -Approval unapproved | `
    Where-Object { $_.update.CreationDate -lt $(Get-Date).AddDays(-$days) } | Measure-Object).Count

if (-not($DryRun)) {
    Get-WsusUpdate -Classification all -Approval unapproved | `
    Where-Object { $_.update.CreationDate -lt $(Get-Date).AddDays(-$days) } |
    Approve-wsusupdate -Targetgroup $targetgroupname -Action 'Install'
}    

if ($DryRun) {
    Write-Output "Dryrun! No actions performed"
}
Write-Output "Number of updates approved to Install : $n"
