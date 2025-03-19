telegraf-config-witness:
    file.managed:
        - name: 'C:\ProgramData\Telegraf\telegraf.conf'
        - source: salt://{{ slspath }}/conf/telegraf.conf
        - template: jinja
        - require:
            - file: telegraf-config-dir
        - require_in:
            - mdb_windows: telegraf-service-installed
            - mdb_windows: telegraf-service-running
        - onchanges_in:
            - mdb_windows: telegraf-restart

