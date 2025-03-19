sqlserver-tasks-req:
    test.nop

sqlserver-tasks-ready:
    test.nop

runbackup-file-present:
    file.managed:
        - name: "C:\\Program Files\\Mdb\\RunBackup.ps1"
        - template: jinja
        - source: salt://components/sqlserver/conf/RunBackup.ps1
        - require:
            - test: sqlserver-tasks-req

{% set backup_hrs = salt['pillar.get']('data:backup:start:hours', 22)|int %}
{% set backup_minutes = salt['pillar.get']('data:backup:start:minutes', 0)|int %}

full-backup-scheduled:
    mdb_windows_tasks.present:
        - name: full-db_backup
        - command: 'powershell.exe'
        - arguments: '-file "C:\Program Files\Mdb\RunBackup.ps1" -Action backup-push'
        - schedule_type: 'Daily'
        - start_time: '{{ "%02d:%02d:00"|format(backup_hrs, backup_minutes) }}'
        - days_interval: 1
        - location: database_maintenance
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True
        - require:
            - file: "C:\\Program Files\\Mdb\\RunBackup.ps1"
            - test: sqlserver-tasks-req
        - require_in:
            - test: sqlserver-tasks-ready

log-backup-scheduled:
    mdb_windows_tasks.present:
        - name: log_backup
        - command: 'powershell.exe'
        - arguments: '-file "C:\Program Files\Mdb\RunBackup.ps1" -Action log-push'
        - schedule_type: 'Once'
        - start_time: '00:00:00'
        - repeat_interval: "15 minutes"
        - location: database_maintenance
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True
        - require:
            - file: "C:\\Program Files\\Mdb\\RunBackup.ps1"
            - test: sqlserver-tasks-req
        - require_in:
            - test: sqlserver-tasks-ready

purgebackuphistory-file-present:
    file.managed:
        - name: "C:\\Program Files\\Mdb\\PurgeBackupHistory.ps1"
        - template: jinja
        - source: salt://components/sqlserver/conf/PurgeBackupHistory.ps1
        - require:
            - test: sqlserver-tasks-req

backup-purge-scheduled:
    mdb_windows_tasks.present:
        - name: purge-backup-history
        - command: 'powershell.exe'
        - arguments: '-file "C:\Program Files\Mdb\PurgeBackupHistory.ps1" -Days 30'
        - schedule_type: 'Daily'
        - start_time: '{{ "%02d:%02d:00"|format(backup_hrs, backup_minutes) }}'
        - days_interval: 1
        - location: database_maintenance
        - enabled: True
        - multiple_instances: "No New Instance"
        - force_stop: True
        - require:
            - file: "C:\\Program Files\\Mdb\\PurgeBackupHistory.ps1"
            - test: sqlserver-tasks-req
        - require_in:
            - test: sqlserver-tasks-ready
