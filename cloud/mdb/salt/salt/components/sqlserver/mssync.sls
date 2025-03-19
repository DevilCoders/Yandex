
mssync-config-dir:
    file.directory:
        - name: 'C:\ProgramData\MSSync'
        - user: 'SYSTEM'

mssync-config:
    file.managed:
        - name: 'C:\ProgramData\MSSync\mssync.yaml'
        - source: salt://{{ slspath }}/conf/mssync.yaml
        - template: jinja
        - require:
            - file: mssync-config-dir

mssync-service-installed:
    mdb_windows.service_installed:
        - service_name: 'mssync'
        - service_call: 'C:\Program Files\MSSync\mssync.exe'
        - service_args: ['--config-path', 'C:/ProgramData/mssync']
        - require:
            - file: mssync-config
            - mdb_windows: mssync-package 
            - mdb_windows: nssm-package 
            - mdb_windows: set-system-path-environment-nssm
        - require_in:
            - mdb_windows: mssync-restart

mssync-restart:
    mdb_windows.service_restarted:
        - service_name: 'MSSync'
        - onchanges:
            - file: mssync-config
