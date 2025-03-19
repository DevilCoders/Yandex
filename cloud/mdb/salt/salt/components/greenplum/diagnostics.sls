{% from "components/greenplum/map.jinja" import gpdbvars with context %}

diagnostic-packages:
    pkg.installed:
        - pkgs:
            - libc6-dbg
            - gdb
            - strace
            - ltrace

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

diagnosctic-sqls-dir:
    file.directory:
        - name: /home/gpadmin/.sql
        - makedirs: True
        - user: gpadmin
        - group: gpadmin
        - mode: 755

diagnostics-sqls:
    file.recurse:
        - name: /home/gpadmin/.sql
        - file_mode: '0755'
        - template: jinja
        - source: salt://{{ slspath }}/conf/sql
        - include_empty: True
        - require:
            - file: diagnosctic-sqls-dir
