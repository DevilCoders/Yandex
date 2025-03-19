{% set version_string = salt.pillar.get("data:sqlserver:version:major_human") %}
{% set folder_name = salt['mdb_sqlserver.get_folder_by_version'](version_string) %}

datadir-initialized:
    cmd.run:
        - shell: powershell
        - name: >
            Expand-Archive -Path 'C:\datadir.zip' -DestinationPath 'D:\';
            Remove-Item -Path D:\SqlServer\*\MSSQL\Log\*
        - unless: >
            Get-Item -Path D:\SqlServer
        - require:
            - cmd: disk-d-mounted
            - test: sqlserver-firewall-ready
        - require_in:
            - test: sqlserver-service-req
            
datadir-permissions:
    cmd.run:
        - shell: powershell
        - name: >
            $acl = Get-Acl -Path 'D:\SqlServer';
            $rule = New-Object System.Security.AccessControl.FileSystemAccessRule("NT Service\MSSQLSERVER", "FullControl", "ContainerInherit,ObjectInherit", "None", "Allow");
            $acl.SetAccessRule($rule);
            Set-Acl -Path 'D:\SqlServer' $acl
        - unless: >
            $acl = Get-Acl -Path 'D:\SqlServer';
            $rule = $acl.Access | Where { $_.IdentityReference -eq 'NT Service\MSSQLSERVER' };
            exit [int]($null -eq $rule)
        - require:
            - cmd: datadir-initialized
        - require_in:
            - test: sqlserver-service-req

{% set password = salt['pillar.get']('data:sqlserver:users:sa:password') %}
sa-pwd-set:
    mdb_sqlserver.login_present:
        - name: sa
        - password: {{password|yaml_encode}}
        - sid: 0x01
        - Trusted_Connection: 'Yes'
        - require: 
            - test: windows-cluster-ready
            - mdb_windows: sqlserver-service
        - require_in:
            - mdb_sqlserver: sqlserver-hostname-match
            - test: sqlserver-logins-ready

sqlserver-hostname-match:
    mdb_sqlserver.hostname_matches:
        - restart: false
        - require: 
            - test: windows-cluster-ready
            - mdb_windows: sqlserver-service
            - mdb_sqlserver: change-sqlserver-collation
        - require_in:
            - test: sqlserver-service-ready
            - mdb_sqlserver: sqlserver-wait-started

sqlserver-hadrenabled:
    cmd.run:
        - shell: powershell
        - name: >
            import-module sqlps;
            Enable-SqlalwaysOn -Path "SQLSERVER:\SQL\$env:computername\DEFAULT" -NoServiceRestart
        - unless: >
            import-module sqlps;
            $enabled = (Get-Item -Path "SQLSERVER:\SQL\$env:computername\DEFAULT").IsHadrEnabled;
            exit [int]($false -eq $enabled)
        - require: 
            - mdb_sqlserver: change-sqlserver-collation
            - test: windows-cluster-ready
            - mdb_windows: sqlserver-service
        - require_in:
            - test: sqlserver-service-ready
            - mdb_sqlserver: sqlserver-wait-started

sqlserver-smk-saved:
    cmd.run:
        - shell: powershell
        - name: Backup-SqlSMK -EncryptionPassword {{password|yaml_encode}}
        - require: 
            - mdb_windows: sqlserver-service
            - file: mdbsqlsrv-module-present
        - unless: ls D:\SqlServer\{{folder_name}}\MSSQL\backup\SMK.key

sqlserver-smk-restored:
    cmd.run:
        - shell: powershell
        - name: Restore-SqlSMK -DencryptionPassword {{password|yaml_encode}}
        - unless: ls "C:\Program Files\Microsoft SQL Server\smk_restored"
        - require:
            - cmd: sqlserver-smk-saved
            - mdb_windows: sqlserver-service
            - file: mdbsqlsrv-module-present
        - require_in:
            - test: sqlserver-service-ready

{% set collation = salt['pillar.get']('data:sqlserver:sqlcollation','Cyrillic_General_CI_AS')%}
       
change-sqlserver-collation:
    mdb_sqlserver.sqlcollation_matches:
        - name: {{salt['pillar.get']('data:sqlserver:sqlcollation','Cyrillic_General_CI_AS') | yaml_encode}}
        - sqlserver_version: {{ salt['pillar.get']('data:sqlserver:version:major_human') | yaml_encode }}
        - sa_password: {{password|yaml_encode}}
        - require:
            - mdb_windows: sqlserver-service
        - require_in:
            - test: sqlserver-service-ready
            - cmd: sqlserver-smk-saved

restart-sqlserver-service:
    service.running:
        - name: MSSQLSERVER
        - reload: True
        - require:
            - cmd: sqlserver-hadrenabled
            - mdb_sqlserver: sqlserver-hostname-match
        - watch:
            - cmd: sqlserver-hadrenabled
            - mdb_sqlserver: sqlserver-hostname-match
        - require_in: 
            - test: sqlserver-service-ready


sqlserver-service-recovery-set:
    mdb_windows.service_recovery_set:
        - name: mssqlserver
        - recovery_action: 'restart'
        - recovery_delay_ms: 10000
        - reset_delay_s: 60
        - require_in: 
            - test: sqlserver-service-ready
