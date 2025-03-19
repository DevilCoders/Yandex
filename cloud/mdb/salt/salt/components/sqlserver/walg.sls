walg-ready:
  test.nop:
    - require:
        - mdb_windows: walg-package
        - file: walg-config
        - file: walg-fake-domain
        - cmd: walg-import-fake-cert
        - mdb_windows: walg-set-system-path-environment

walg-set-system-path-environment:
    mdb_windows.add_to_system_path:
        - path: 'C:\Program Files\wal-g-sqlserver\'

walg-config-dir:
    file.directory:
        - name: 'C:\ProgramData\wal-g'
        - user: 'SYSTEM'

walg-fake-domain:
    file.append:
        - name: 'C:\Windows\System32\Drivers\etc\hosts'
        - text: >
            127.0.0.42  backup.local

walg-fake-cert:
    file.managed:
        - name: 'C:\ProgramData\wal-g\backup.local.cert.pem'
        - source: salt://{{ slspath }}/conf/backup.local.cert.pem
        - require:
            - file: walg-config-dir

walg-fake-key:
    file.managed:
        - name: 'C:\ProgramData\wal-g\backup.local.key.pem'
        - source: salt://{{ slspath }}/conf/backup.local.key.pem
        - require:
            - file: walg-config-dir

walg-import-fake-cert:
    cmd.run:
        - shell: powershell
        - name: >
            Import-Certificate 
            -CertStoreLocation cert:\LocalMachine\Root 
            -FilePath 'C:\ProgramData\wal-g\backup.local.cert.pem'
        - unless: >
            $crt = New-Object System.Security.Cryptography.X509Certificates.X509Certificate;
            $crt.Import('C:\ProgramData\wal-g\backup.local.cert.pem');
            $sn = $crt.GetSerialNumberString();
            $found = Get-ChildItem -Path cert:\LocalMachine\Root | Where-Object { $_.SerialNumber -eq $sn };
            exit [int]($null -eq $found)
        - require:
            - file: walg-fake-cert
            - file: walg-fake-key

walg-fake-credentials-base:
    mdb_sqlserver.query:
        - sql: >
            CREATE CREDENTIAL [https://backup.local/basebackups_005]
            WITH IDENTITY='SHARED ACCESS SIGNATURE', SECRET = 'does_not_matter'
        - unless: >
            SELECT COUNT(*) FROM sys.credentials WHERE NAME = 'https://backup.local/basebackups_005'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: walg-ready

walg-fake-credentials-wal:
    mdb_sqlserver.query:
        - sql: >
            CREATE CREDENTIAL [https://backup.local/wal_005]
            WITH IDENTITY='SHARED ACCESS SIGNATURE', SECRET = 'does_not_matter'
        - unless: >
            SELECT COUNT(*) FROM sys.credentials WHERE NAME = 'https://backup.local/wal_005'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: walg-ready

walg-config:
    file.managed:
        - name: 'C:\ProgramData\wal-g\wal-g.yaml'
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - context:
            restore_config: false
        - require:
            - file: walg-config-dir
            - file: walg-pgp-key

walg-pgp-key:
    file.managed:
        - name: 'C:\ProgramData\wal-g\PGP_KEY'
        - contents: |
            {{salt['pillar.get']('data:s3:gpg_key') | indent(12)}}
        - require:
            - file: walg-config-dir

walg-service-installed:
    mdb_windows.service_installed:
        - service_name: 'walg-proxy'
        - service_call: 'C:\Program Files\wal-g-sqlserver\wal-g-sqlserver.exe'
        - service_args: ['proxy', '--config', 'C:\ProgramData\wal-g\wal-g.yaml']
        - service_settings:
            AppStdout: 'C:\Logs\walg-proxy.log'
            AppStderr: 'C:\Logs\walg-proxy.log'
        - require:
            - file: walg-config
            - mdb_windows: walg-package 
            - mdb_windows: nssm-package 
            - mdb_windows: set-system-path-environment-nssm

walg-service-running:
    mdb_windows.service_running:
        - service_name: "walg-proxy"
        - require:
            - mdb_windows: walg-service-installed

walg-service-restart:
    mdb_windows.service_restarted:
        - service_name: 'walg-proxy'
        - require:
            - mdb_windows: walg-service-installed
        - onchanges:
            - file: walg-config
            - mdb_windows: walg-package

{% if salt['pillar.get']('restore-from:cid') %}

walg-fake-restore-domain:
    file.append:
        - name: 'C:\Windows\System32\Drivers\etc\hosts'
        - text: >
            127.0.0.43  restore.local
        - require_in:
            - test: walg-ready

walg-fake-restore-cert:
    file.managed:
        - name: 'C:\ProgramData\wal-g\restore.local.cert.pem'
        - source: salt://{{ slspath }}/conf/restore.local.cert.pem
        - require:
            - file: walg-config-dir

walg-fake-restore-key:
    file.managed:
        - name: 'C:\ProgramData\wal-g\restore.local.key.pem'
        - source: salt://{{ slspath }}/conf/restore.local.key.pem
        - require:
            - file: walg-config-dir

walg-import-restore-fake-cert:
    cmd.run:
        - shell: powershell
        - name: >
            Import-Certificate 
            -CertStoreLocation cert:\LocalMachine\Root 
            -FilePath 'C:\ProgramData\wal-g\restore.local.cert.pem'
        - unless: >
            $crt = New-Object System.Security.Cryptography.X509Certificates.X509Certificate;
            $crt.Import('C:\ProgramData\wal-g\restore.local.cert.pem');
            $sn = $crt.GetSerialNumberString();
            $found = Get-ChildItem -Path cert:\LocalMachine\Root | Where-Object { $_.SerialNumber -eq $sn };
            exit [int]($null -eq $found)
        - require:
            - file: walg-fake-restore-cert
            - file: walg-fake-restore-key
        - require_in:
            - test: walg-ready

walg-fake-credentials-base-restore:
    mdb_sqlserver.query:
        - sql: >
            CREATE CREDENTIAL [https://restore.local/basebackups_005]
            WITH IDENTITY='SHARED ACCESS SIGNATURE', SECRET = 'does_not_matter'
        - unless: >
            SELECT COUNT(*) FROM sys.credentials WHERE NAME = 'https://restore.local/basebackups_005'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: walg-ready

walg-fake-credentials-wal-restore:
    mdb_sqlserver.query:
        - sql: >
            CREATE CREDENTIAL [https://restore.local/wal_005]
            WITH IDENTITY='SHARED ACCESS SIGNATURE', SECRET = 'does_not_matter'
        - unless: >
            SELECT COUNT(*) FROM sys.credentials WHERE NAME = 'https://restore.local/wal_005'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: walg-ready

walg-restore-config:
    file.managed:
        - name: 'C:\ProgramData\wal-g\wal-g-restore.yaml'
        - template: jinja
        - source: salt://{{ slspath }}/conf/wal-g.yaml
        - context:
            restore_config: true
        - require:
            - file: walg-config-dir
            - file: walg-restore-pgp-key
        - require_in:
            - test: walg-ready

walg-restore-pgp-key:
    file.managed:
        - name: 'C:\ProgramData\wal-g\RESTORE_PGP_KEY'
        - contents: |
            {{salt['pillar.get']('data:restore-from-pillar-data:s3:gpg_key') | indent(12)}}
        - require:
            - file: walg-config-dir
{% endif %}

