param (
        $warn = 2,
        $crit = 4
)

$res = wal-g-sqlserver --config C:\ProgramData\wal-g\wal-g.yaml backup-list
$res = ConvertFrom-Csv -Delimiter ' ' ($res -replace '\s+', ' ')

if ($res.Length -eq 0) {
        echo "2;No backups found"
        exit 0
}
$age = ([datetime]::now - [datetime]($res[-1].modified)).Days
if ($age -gt $crit) {
        echo "2;Last backup was $age days ago"
        exit 0
}
if ($age -gt $warn) {
        echo "1;Last backup was $age days ago"
        exit 0
}
echo "0;OK"
