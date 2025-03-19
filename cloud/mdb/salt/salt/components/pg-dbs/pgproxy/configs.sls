{% from "components/postgres/pg.jinja" import pg with context %}
/etc/logrotate.d/pgcheck:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/pgcheck-logrotate
        - user: root
        - group: root
        - mode: 644

/etc/mdb-pgcheck.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-pgcheck.yaml
        - user: root
        - group: root
        - mode: 644

/lib/systemd/system/mdb-pgcheck.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-pgcheck.service
        - require:
            - pkg: mdb-pgcheck
            - file: /etc/mdb-pgcheck.yaml
        - onchanges_in:
            - module: systemd-reload

/etc/supervisor/conf.d/ping_pgbouncer.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor_ping_pgbouncer.conf
        - user: root
        - group: root
        - file_mode: 644

/etc/cron.d/wd-pgproxy-ping:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/wd-pgproxy-ping.sh
        - mode: 644
        - require:
            - file: /etc/supervisor/conf.d/ping_pgbouncer.conf

{% if pg.connection_pooler == 'pgbouncer' and salt['pillar.get']('data:pgbouncer:count', 1) > 1 %}
supervisor-config-restart-ping:
    cmd.wait:
        - name: (supervisorctl update && supervisorctl restart ping_pgbouncer) || true
        - watch:
            - file: /etc/supervisor/supervisord.conf
            - file: /etc/supervisor/conf.d/ping_pgbouncer.conf
            - file: /usr/local/yandex/webserver_check_pgbouncer.py
{% endif %}
