{% set supervisor_version = '3.3.1-1yandex' %}

supervisor:
    pkg:
        - installed
        - version: {{ supervisor_version }}
    service:
        - running
        - enable: True
        - require:
            - pkg: supervisor

stop_pgbouncer:
    cmd.run:
        - name: kill $(ps aux | grep pgbouncer | grep -v grep | awk '{print $2};') || true
        - runas: root
        - group: root
        - require:
            - pkg: supervisor
            - pkg: pgbouncer-pkg
        - unless: grep supervisor /etc/init.d/pgbouncer

supervisor-config-restart:
    cmd.wait:
        - name: (supervisorctl restart prestart_pgbouncer && supervisorctl update) || true
        - require_in:
            - service: pgbouncer
        - watch:
            - file: /etc/supervisor/supervisord.conf
            - file: /etc/supervisor/conf.d/pgbouncer.conf

/var/log/supervisor:
    file.directory:
        - makedirs: True
        - require_in:
            - cmd: stop_pgbouncer
            - service: pgbouncer

/etc/supervisor/supervisord.conf:
    file.managed:
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require_in:
            - service: pgbouncer
            - cmd: stop_pgbouncer

/etc/supervisor/conf.d/pgbouncer.conf:
    file.managed:
        - template: jinja
        - makedirs: True
        - source: salt://{{ slspath }}/conf/pgbouncer.conf
        - require:
            - file: /usr/local/bin/prestart_pgbouncer.sh
        - require_in:
            - service: pgbouncer
            - cmd: stop_pgbouncer

/etc/init.d/pgbouncer:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgbouncer_supervisor.init
        - follow_symlinks: False
        - mode: 755
        - require:
            - pkg: supervisor
            - cmd: stop_pgbouncer
        - require_in:
            - service: pgbouncer

/usr/local/bin/prestart_pgbouncer.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/prestart_pgbouncer.sh
        - mode: 0755
        - require_in:
            - service: pgbouncer
            - file: /etc/supervisor/conf.d/pgbouncer.conf

/etc/cron.d/wd-habouncer:
    file.managed:
        - source: salt://{{ slspath }}/conf/cron.d/wd-habouncer
        - require:
            - service: pgbouncer

include:
    - .habouncer

extend:
    postgresql-habouncer-req:
        test.nop:
            - require:
                - service: supervisor

    postgresql-habouncer-ready:
        test.nop:
            - require_in:
                - service: pgbouncer
