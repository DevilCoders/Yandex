{% from "components/mysql/map.jinja" import mysql with context %}

mysql-slow-shutdown-enabled:
    cmd.run:
        - name: >
            mysql --defaults-file={{ mysql.defaults_file }} -e 'SET GLOBAL innodb_fast_shutdown = 0'
            && touch /tmp/.mysql_upgrade_required
        - unless: >
            ! pidof mysqld
            || test "{{ mysql.version.minor }}" = `mysql --defaults-file={{ mysql.defaults_file }} -Ne "select concat(sys.version_major(), '.', sys.version_minor(), '.', sys.version_patch())"`
        - require_in:
            - service: mysql-stopped

mysql-stopped:
    service.dead:
        - name: mysql
        - require:
            - file: /etc/mysql/my.cnf
        - require_in:
            - test: mysql-service-req

mysql-upgraded:
    cmd.run:
        - name: >
{% if mysql.version.num < 800 %}
            /usr/local/yandex/mysql_upgrade.py --defaults-file={{ mysql.defaults_file }} --port=3308
{% else %}
            true
{% endif %}
            && rm /tmp/.mysql_upgrade_required
            && touch /tmp/.mysql_upgrade_done
        - onlyif: >
            ls /tmp/.mysql_upgrade_required 2>/dev/null
        - require:
            - service: mysql-stopped
        - require_in:
            - service: mysql-service

mysql-wait-synced:
    cmd.wait:
        - name: my-wait-synced --wait {{ salt['pillar.get']('post-restart-timeout', 300) | int }}s --defaults-file={{ mysql.defaults_file }} --replica-lag={{ salt['pillar.get']('data:mysql:config:mdb_offline_mode_disable_lag', 300) }}s
        - require:
            - cmd: mysql-wait-started
        - require_in:
            - test: mysql-service-ready

mysql-set-online:
    cmd.run:
        - name: mysql --defaults-file={{ mysql.defaults_file }} --execute 'SET GLOBAL offline_mode = 0'
        - require:
            - cmd: mysql-wait-synced
        - require_in:
            - test: mysql-service-ready
