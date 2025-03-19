sqlserver-agconfig-ready:
    test.nop

repl-cert-crt:
  file.managed:
      - name: 'C:\Program Files\Microsoft SQL Server\repl.crt'
      - contents_pillar: data:sqlserver:repl_cert:cert.crt
      - user: SYSTEM
      - require_in:
        - test: sqlserver-agconfig-ready

repl-cert-key:
  file.managed:
      - name: 'C:\Program Files\Microsoft SQL Server\repl.key'
      - contents_pillar: data:sqlserver:repl_cert:cert.key
      - user: SYSTEM
      - require_in:
        - test: sqlserver-agconfig-ready

repl-cert-der:
  cmd.run:
    - shell: powershell
    - name: >
        & 'C:\Program Files\openssl\openssl.exe' x509 -outform DER
        -in 'C:\Program Files\Microsoft SQL Server\repl.crt'
        -out 'C:\Program Files\Microsoft SQL Server\repl.der'
    - require:
        - mdb_windows: openssl-package
    - onchanges:
        - file: repl-cert-crt
    - require_in:
        - test: sqlserver-agconfig-ready

repl-cert-pvk:
  cmd.run:
    - shell: powershell
    - name: >
        & 'C:\Program Files\openssl\openssl.exe' rsa -outform PVK -pvk-strong -passout pass:Password1!
        -in 'C:\Program Files\Microsoft SQL Server\repl.key'
        -out 'C:\Program Files\Microsoft SQL Server\repl.pvk'
    - require:
        - mdb_windows: openssl-package
    - onchanges:
        - file: repl-cert-key
    - require_in:
        - test: sqlserver-agconfig-ready

ag_master_key_present:
    mdb_sqlserver.query:
        - sql: >
            CREATE MASTER KEY ENCRYPTION BY PASSWORD = 'Password1!'
        - unless: >
            SELECT COUNT(*) FROM sys.symmetric_keys WHERE name like '##MS_DatabaseMasterKey##'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: sqlserver-agconfig-ready

ag_cert_present:
    mdb_sqlserver.query:
        - sql: >
            CREATE CERTIFICATE AG_Cert 
            FROM FILE = 'C:\Program Files\Microsoft SQL Server\repl.der'
            WITH PRIVATE KEY (FILE = 'C:\Program Files\Microsoft SQL Server\repl.pvk', DECRYPTION BY PASSWORD = 'Password1!') 
        - unless: >
            SELECT COUNT(*) FROM sys.certificates where name like 'AG_Cert'
        - require:
            - test: sqlserver-service-ready
            - mdb_sqlserver: ag_master_key_present
            - cmd: repl-cert-der
            - cmd: repl-cert-pvk
        - require_in:
            - test: sqlserver-agconfig-ready

ag_login_present:
    mdb_sqlserver.query:
        - sql: >
            CREATE LOGIN AG_LOGIN FROM CERTIFICATE AG_Cert
        - unless: >
            SELECT COUNT(*) FROM sys.server_principals WHERE name like 'AG_LOGIN'
        - require:
            - test: sqlserver-service-ready
            - mdb_sqlserver: ag_cert_present
        - require_in:
            - test: sqlserver-agconfig-ready

ag_login_privs:
    mdb_sqlserver.query:
        - sql: >
            ALTER SERVER ROLE sysadmin ADD MEMBER AG_LOGIN 
        - unless: >
            SELECT COUNT(*) FROM sys.syslogins WHERE name = 'AG_LOGIN' and sysadmin = 1
        - require:
            - test: sqlserver-service-ready
            - mdb_sqlserver: ag_login_present
        - require_in:
            - test: sqlserver-agconfig-ready

ag_endpoint_present:
    mdb_sqlserver.hadr_endpoint_present:
        - name: AG_ENDPOINT
        - port: 5022
        - cert_name: AG_Cert
        - encryption: AES
        - require:
            - mdb_sqlserver: ag_login_privs
        - require_in:
            - test: sqlserver-agconfig-ready

ag_tracing_enabled:
    mdb_sqlserver.query:
        - sql: >
            ALTER EVENT SESSION AlwaysOn_health ON SERVER STATE=START;
            ALTER EVENT SESSION [AlwaysOn_health] ON SERVER WITH (STARTUP_STATE=ON)
        - unless: >
            SELECT count(*) FROM sys.dm_xe_sessions WHERE name like 'AlwaysOn_health'
        - require:
            - test: sqlserver-service-ready
        - require_in:
            - test: sqlserver-agconfig-ready
