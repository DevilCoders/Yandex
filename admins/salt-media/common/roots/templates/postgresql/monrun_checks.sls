{% from "templates/postgresql/map.jinja" import postgres with context %}

monrun-pg-common-scripts:
  file.recurse:
    - name: /usr/local/yandex/monitoring
    - file_mode: '0755'
    - template: jinja
    - source: salt://templates/postgresql/files/pg_checks
    - include_empty: True
    {% if postgres.monrun_checks.use_repl_mon == False %}
    - exclude_pat: pg_replication*.conf
    {% endif %}

monrun-pg-common-confs:
  file.recurse:
    - name: /etc/monrun/conf.d
    - file_mode: '0644'
    - template: jinja
    - source: salt://templates/postgresql/files/monrun/conf.d
    - include_empty: True
    {% if postgres.monrun_checks.use_repl_mon == False %}
    - exclude_pat: pg_replication*.conf
    {% endif %}

monrun-pg-common-sudoers:
  file.recurse:
    - name: /etc/sudoers.d
    - file_mode: '0640'
    - template: jinja
    - source: salt://templates/postgresql/files/sudoers.d
    - include_empty: True

monrun-pg-common-jobs-update:
  cmd.wait:
    - name: /usr/bin/monrun --gen-jobs
    - watch:
        - file: monrun-pg-common-confs
        - file: monrun-pg-common-scripts
        - file: monrun-pg-common-sudoers

{% if postgres.monrun_checks.use_repl_mon == True %}
monrun-repl_mon-lib:
  file.managed:
    - name: /usr/lib/postgresql/{{ postgres.version }}/lib/repl_mon.so
    - source: salt://templates/postgresql/files/{{ postgres.version }}/lib/repl_mon.so
    - replace: true
    - user: root
    - group: root
    - mode: 644
{% endif %}
