{% from "components/postgres/pg.jinja" import pg with context %}

diagnostic-packages:
    pkg.installed:
        - pkgs:
            - libc6-dbg
            - gdb
            - strace
            - ltrace

kernel-dbgsym-purged:
    pkg.purged:
        - pkgs:
            - linux-image-4.9.101-26-dbgsym

/usr/local/yandex/gdb_bt_cmd:
    file.managed:
        - source: salt://{{ slspath }}/conf/gdb_bt_cmd
        - user: root
        - group: root
        - mode: 644
        - makedirs: True
        - require:
            - pkg: diagnostic-packages

/usr/bin/bt:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/bt.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - pkg: diagnostic-packages
            - file: /usr/local/yandex/gdb_bt_cmd
            - service: postgresql-service

diagnosctic-sqls-dir:
    file.directory:
        - name: {{ pg.prefix }}/.sql
        - makedirs: True
        - user: postgres
        - group: postgres
        - mode: 755

diagnostics-sqls:
    file.recurse:
        - name: {{ pg.prefix }}/.sql
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/conf/sql
        - include_empty: True
        - require:
            - file: diagnosctic-sqls-dir
