param (  
    [string]$walg_path = "C:\Program Files\wal-g-sqlserver\wal-g-sqlserver.exe",
    [string]$config_path = "C:\ProgramData\wal-g\wal-g.yaml",
    [string]$action = "backup-push"
)

$replica_status_sql = "
    WITH replica_status as (
        SELECT 
            COUNT(*) as cnt
          FROM sys.dm_hadr_database_replica_cluster_states
          WHERE is_database_joined = 0
        UNION ALL
        SELECT 
            COUNT(*) as cnt
          FROM sys.dm_hadr_database_replica_states
          WHERE is_commit_participant = 0
        )
        SELECT
            SUM(cnt)
          FROM replica_status as cnt
    "

$is_replica = (Invoke-Sqlcmd msdb.dbo.mdb_is_replica).is_replica

$log_path = "C:\Logs\walg-backups.log"
if ($action -eq 'log-push') {
    $log_path = "C:\Logs\walg-logs.log"
}

function Run-WalG() {
    & $walg_path --config $config_path @Args 2>&1 | %{ "$_" } >>$log_path
}

if ($is_replica) {
    Write-Output "$(Get-Date) Backup halted due to non-master replica" *>> $log_path
    exit
}

if ($action -eq 'backup-push') {
    Write-Output "$(Get-Date) BACKUP PUSH" *>> $log_path
    Run-WalG backup-push

    $untilDate = (Get-Date).AddDays(-7).ToString("yyyy-MM-ddTHH:mm:ssZ")
    Write-Output "$(Get-Date) DELETE UNTIL $untilDate" *>> $log_path
    Run-WalG delete before FIND_FULL $untilDate --confirm
}

if ($action -eq 'log-push') {
    Write-Output "$(Get-Date) LOG PUSH" *>> $log_path
    if ((Invoke-Sqlcmd $replica_status_sql).cnt -gt 0) {
        Write-Output "$(Get-Date) Disjoined replicas exist. Need to preserve logs." *>> $log_path
        exit
    }
    Run-WalG log-push
}

