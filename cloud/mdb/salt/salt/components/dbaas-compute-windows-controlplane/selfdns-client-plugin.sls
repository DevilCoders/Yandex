selfdns-client-data-present:
    file.directory:
        - name: 'C:\ProgramData\yandex\selfdns-client-data'
        - user: SYSTEM
        - require:
            - mdb_windows: selfdns-client-installed

plugin-folder-present:
    file.directory:
        - name: 'C:\ProgramData\yandex\selfdns-client-data\plugins'
        - user: SYSTEM
        - require:
            - mdb_windows: selfdns-client-installed
            - file: selfdns-client-data-present

plugin-ps1-present:
    file.managed:
        - name: 'C:\ProgramData\yandex\selfdns-client-data\plugins\ipv6only_w.ps1'
        - source: salt://{{slspath}}/conf/yandex/selfdns-client/plugins/ipv6only_w.ps1
        - template: jinja
        - require:
            - file: plugin-folder-present

plugin-com-present:
    file.managed:
        - name: 'C:\ProgramData\yandex\selfdns-client-data\plugins\ipv6only_w.cmd'
        - source: salt://{{slspath}}/conf/yandex/selfdns-client/plugins/ipv6only_w.cmd
        - template: jinja
        - require:
            - file: plugin-folder-present
