telegraf-config-sqlserver:
    file.managed:
        - name: 'C:\ProgramData\Telegraf\telegraf.conf'
        - source: salt://{{ slspath }}/conf/telegraf_sqlserver.conf
        - template: jinja
        - require:
            - file: telegraf-config-dir
        - require_in:
            - mdb_windows: telegraf-service-installed
            - mdb_windows: telegraf-service-running
        - onchanges_in:
            - mdb_windows: telegraf-restart

telegraf-checks-sqlserver:
    file.recurse:
        - name: 'C:\Program Files\Mdb\juggler\'
        - template: jinja
        - source: salt://{{ slspath }}/scripts
        - include_empty: True
