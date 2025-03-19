include:
    - .iam_token_reissuer
    - .packages


telegraf-config-dir:
    file.directory:
        - name: 'C:\ProgramData\Telegraf'
        - user: 'SYSTEM'

# telegraf-config:
#     file.managed:
#         - name: 'C:\ProgramData\Telegraf\telegraf.conf'
#         - source: salt://{{ slspath }}/conf/telegraf.conf
#         - template: jinja
#         - require:
#             - file: telegraf-config-dir

{% if salt['pillar.get']('data:cluster_private_key') %}
telegraf-cluster-key:
    file.managed:
        - name: 'C:\ProgramData\Telegraf\cluster_key.pem'
        - contents_pillar: data:cluster_private_key
        - require:
            - file: telegraf-config-dir
{% endif %}

telegraf-service-installed:
    mdb_windows.service_installed:
        - service_name: 'telegraf'
        - service_call: 'C:\Program Files\Telegraf\telegraf.exe'
        - service_args: ['--console', '--config', 'C:\ProgramData\Telegraf\telegraf.conf']
        - require:
            - mdb_windows: telegraf-package 
            - mdb_windows: nssm-package 
            - mdb_windows: set-system-path-environment-nssm

telegraf-service-running:
    mdb_windows.service_running:
        - service_name: "telegraf"
        - require:
            - mdb_windows: telegraf-service-installed

telegraf-restart:
    mdb_windows.service_restarted:
        - service_name: 'telegraf'
        - require:
            - mdb_windows: telegraf-service-installed

telegraf-set-system-path-environment:
    mdb_windows.add_to_system_path:
        - path: 'C:\Program Files\Telegraf\'

juggler-checks-config-dir:
    file.directory:
        - name: 'C:\Program Files\MDB\juggler'
        - win_user: 'Administrator'

juggler-checks-scripts:
    file.recurse:
        - name: 'C:\Program Files\MDB\juggler'
        - template: jinja
        - source: salt://{{ slspath }}/conf/juggler_checks
        - include_empty: True
        - user: 'Administrator'
        - require:
            - file: juggler-checks-config-dir
