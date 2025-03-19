include:
    - .config

mdb-billing-package:
    mdb_windows.nupkg_installed:
       - name: MdbBilling
       - version: '1.9468221.0'
       - require:
           - cmd: mdb-repo
       - onchanges_in:
           - mdb_windows: billing-restart

billing-config-dir:
    file.directory:
        - name: 'C:\ProgramData\MdbBilling'
        - user: 'SYSTEM'

billing-data-dir:
    file.directory:
        - name: 'C:\ProgramData\MdbBilling\data'
        - user: 'SYSTEM'
        - require:
            - file: billing-config-dir
        - require_in:
            - file: billing-config

billing-service-installed:
    mdb_windows.service_installed:
        - service_name: 'MdbBilling'
        - service_call: 'C:\Program Files\MdbBilling\billing.exe'
        - service_args: ['--config-path', 'C:\ProgramData\MdbBilling']
        - require:
            - file: billing-config
            - mdb_windows: mdb-billing-package 
            - mdb_windows: nssm-package 
            - mdb_windows: set-system-path-environment-nssm
        - require_in:
            - mdb_windows: billing-restart

{% if salt['pillar.get']('data:billing:ship_logs', True) %}

billing-service-running:
    mdb_windows.service_running:
        - service_name: "MdbBilling"
        - require:
            - mdb_windows: billing-firewall
            - file: billing-config
            - mdb_windows: billing-service-installed

{% else %}

billing-service-stopped:
    mdb_windows.service_stopped:
        - service_name: "MdbBilling"
        - require:
            - file: billing-config
            - mdb_windows: billing-service-installed

{% endif %}
