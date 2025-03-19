$sqlroot = "D:\SqlServer\$(ls D:\SqlServer\|Sort-Object -Property Name -Descending|Select Name -Exp Name -First 1)\MSSQL"

function Get-Errorlog {
    cat $sqlroot\Log\Errorlog
}

function Get-Ag {
    invoke-sqlcmd "select ag.name, gs.primary_replica, gs.synchronization_health_desc
        from
        sys.availability_groups ag
        join sys.dm_hadr_availability_group_states gs on ag.group_id = gs.group_id"|ft

}

function Get-AgReplicas {
    Invoke-sqlcmd "SELECT ar.replica_server_name,
            rs.role_desc as role,
            rs.connected_state_desc as connection_state,
            rs.synchronization_health_desc as synchronization_health,
            rs.recovery_health_desc as recovery_health,
            rs.operational_state_desc as operational_state,
            rs.last_connect_error_description
        FROM sys.availability_replicas ar
        JOIN sys.dm_hadr_availability_replica_states rs
            ON rs.replica_id = ar.replica_id
    "|ft
}

function Get-AGDatabases {
    param(
        [bool] $Detailed = $false
    )
    $dbs = invoke-sqlcmd "SELECT ar.replica_server_name,
            db_name(rs.database_id) as database_name,
            rs.is_local,
            is_primary_replica,
            rs.is_commit_participant,
            rs.synchronization_state_desc,
            rs.synchronization_health_desc,
            rs.database_state_desc,
            rs.is_suspended,
            rs.suspend_reason_desc,
            rs.recovery_lsn,
            truncation_lsn,
            last_sent_lsn,
            last_sent_time,
            last_received_lsn,
            last_received_time,
            last_hardened_lsn,
            last_hardened_time,
            last_redone_lsn,
            last_redone_time,
            log_send_queue_size,
            log_send_rate,
            redo_queue_size,
            redo_rate,
            end_of_log_lsn,
            last_commit_lsn,
            last_commit_time,
            secondary_lag_seconds
        FROM
        sys.availability_replicas ar
        JOIN sys.dm_hadr_database_replica_states rs
        ON ar.replica_id = rs.replica_id
    "
    if ($Detailed) {
        $dbs
    }
    else {
        $dbs|select replica_server_name, database_name, is_local, is_primary_replica, synchronization_state_desc, is_suspended, secondary_lag_seconds|ft
    }
}

function Get-Databases {
    param(
        [bool] $Detailed = $false
    )
    $dbs = invoke-sqlcmd "SELECT * FROM sys.databases"
    if ($Detailed) {
        $dbs
    }
    else {
        $dbs|select name, state_desc, user_access_desc, is_read_only, recovery_model_desc, collation_name|ft
    }
}

function hs {
    cmd /c salt-call state.highstate queue=True --state-output=changes $args
}

function wal-g-sqlserver {
    cmd /c 'c:\Program Files\wal-g-sqlserver\wal-g-sqlserver.exe' --config c:\ProgramData\wal-g\wal-g.yaml $args
}

function mont {
    cmd /c 'c:\Program Files\Telegraf\mont.exe' $args
}

function Get-MinionLog {
    cat C:\Salt\var\log\salt\minion
}

function Backup-SqlSMK {
    param(
        [string] $Path = "$sqlroot\Backup\SMK.key",
        [string] $EncryptionPassword
    )
    $sql_command = "BACKUP SERVICE MASTER KEY
                    TO FILE='$Path'
                    ENCRYPTION BY PASSWORD=N'$EncryptionPassword'"
    Invoke-sqlcmd $sql_command
}


function Restore-SqlSMK {
    param(
        [string] $Path = "$sqlroot\Backup\SMK.key",
        [string] $DencryptionPassword,
        [string] $OutFile = 'C:\Program Files\Microsoft SQL Server\smk_restored'
    )
    $sql_command = "RESTORE SERVICE MASTER KEY
                       FROM FILE='$Path'
                       DECRYPTION BY PASSWORD='$DencryptionPassword' FORCE"
    Invoke-sqlcmd $sql_command|Out-File $OutFile
}
$functions = @(
    'Get-Databases',
    'Get-Ag',
    'Get-AgReplicas',
    'Get-AGDatabases',
    'hs',
    'mont',
    'wal-g-sqlserver',
    'Get-Errorlog',
    'Get-MinionLog',
    'Backup-SqlSMK',
    'Restore-SqlSMK'
)
Export-ModuleMember -Function $functions
