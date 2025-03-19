selfdns-config-present:
    file.managed:
        - name: 'C:\ProgramData\yandex\selfdns-client-data\default.conf'
        - source: salt://{{slspath}}/conf/yandex/selfdns-client/default.conf
        - template: jinja
        - require:
            - mdb_windows: selfdns-client-installed

selfdns-client-scheduled:
    mdb_windows_tasks.present:
        - name: selfdns-client
        - command: '"C:\program files\selfdns-client\selfdns-client.exe"'
        - arguments: '--config "C:\ProgramData\yandex\selfdns-client-data\default.conf"'
        - schedule_type: 'Once'
        - repeat_interval: "5 minutes"
        - location: mdb
        - enabled: True
        - multiple_instances: "Parallel"
        - force_stop: True
