param (
    [int] $Days = 30
)
$sql = "
    DECLARE @date datetime
    SELECT @date = DATEADD(DD, -$Days, GETDATE())
    EXEC sp_delete_backuphistory @date"
 
Invoke-DbaQuery -Query $sql -SqlInstance . -Database msdb -ErrorAction SilentlyContinue -ErrorVariable ErrVar

if ($ErrVar){
    Write-Output "ERROR: $(Get-Date) Purging database backup history failed with error: $ErrVar"
}
