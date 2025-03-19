{% from "components/mysql/map.jinja" import mysql with context %}

{% set environment = salt['pillar.get']('yandex:environment', 'dev') %}

include:
    - .gpg
    - .walg-cron
    - .walg-config
{% if salt['pillar.get']('do-backup') %}
    - .run-backup

{% if salt['pillar.get']('restore-from:cid') %}

backup_after_restore_idempotency:
    cmd.run:
        - name: touch /tmp/recovery-state/backup_push_done

{% endif %}

extend:
    do_s3_backup:
        cmd.run:
            - require:
                - test: walg-ready
                - test: mysql-ready
{% if salt['pillar.get']('restore-from:cid') %}
            - require_in:
                - cmd: backup_after_restore_idempotency
            - unless:
                - ls /tmp/recovery-state/backup_push_done
{% endif %}
{% endif %}

/etc/wal-g:
    file.directory:
        - user: root
        - group: s3users
        - mode: 0750
        - makedirs: True
        - require:
            - group: s3users
        - require_in:
            - file: /etc/wal-g/wal-g.yaml

/etc/wal-g/envdir:
    file.absent

walg-packages:
    pkg.installed:
        - pkgs:
{% if environment in ('dev', 'qa') %}
            - wal-g-mysql: '1271-27f437a3'
{% elif environment == 'prod' %}
            - wal-g-mysql: '1271-27f437a3'
{% else %}
            - wal-g-mysql: '1255-afb75585'
{% endif %}
            - daemontools
            - python3-dateutil
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0

# walg installed and all configs created
walg-ready:
  test.nop:
    - require:
        - pkg: walg-packages
        - sls: components.mysql.walg-config
        - sls: components.mysql.walg-cron

/etc/wal-g/PGP_KEY:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:s3:gpg_key') | indent(12)}}
        - require:
            - pkg: walg-packages
            - file: /etc/wal-g/wal-g.yaml
        - require_in:
            - test: walg-ready

/usr/local/yandex/mysql_walg.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_walg.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex
        - require_in:
            - test: walg-ready

/usr/local/yandex/mysql_restore.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_restore.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex
        - require_in:
            - test: walg-ready

s3users:
    group.present:
        - members:
            - mysql
            - monitor
        - system: True
        - require:
            - file: /var/lib/mysql
            - pkg: yamail-monrun
        - watch_in:
            - service: snaked
            - service: juggler-client
        - require_in:
            - file: /etc/wal-g/wal-g.yaml
            - test: walg-ready

{% if salt['pillar.get']('restore-from:cid') %}
/etc/wal-g/RESTORE_PGP_KEY:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt['pillar.get']('data:restore-from-pillar-data:s3:gpg_key') | indent(12)}}
        - require:
            - file: /etc/wal-g/wal-g-restore.yaml
        - require_in:
            - cmd: backup-fetched

remove-keys-when-mysql-restored:
    cmd.run:
        - name: >
            rm -f /etc/wal-g/wal-g-restore.yaml /etc/wal-g/RESTORE_PGP_KEY
        - require:
            - test: mysql-ready

/tmp/recovery-state:
  file.directory:
    - user: mysql
    - group: mysql
    - mode: 0750
    - makedirs: True

{% set memory_guarantee = salt.pillar.get('data:dbaas:flavor:memory_guarantee', 1024**3)|int %}

{% if memory_guarantee >= 16 * 1024**3 %}
{% set prepare_mem_arg = '--use-memory=' + (memory_guarantee // 2)|int|string %}
{% elif memory_guarantee >= 8 * 1024**3 %}
{% set prepare_mem_arg = '--use-memory=' + (memory_guarantee // 4)|int|string %}
{% else %}
{% set prepare_mem_arg = '' %} {# MDB-14526: low memory systems may cause OOM #}
{% endif %}

backup-fetched:
    cmd.run:
        - name: >
            su mysql -c "
            rm -rf /var/lib/mysql/* /var/lib/mysql/.tmp/* &&
            /usr/local/yandex/mysql_restore.py walg-fetch \
                --backup-name=\"{{ salt['pillar.get']('restore-from:backup-id', 'LATEST') }}\" \
                --datadir=/var/lib/mysql \
                --defaults-file={{ mysql.defaults_file }} \
                --walg-config={{ mysql.walg_config }} \
                --max-allowed-packet=1073741824 \
{% if mysql.version.num < 800 %}
                --run-upgrade=1 \
{% endif %}
                {{ prepare_mem_arg }} \
                --port=3308 &&
            (test -d /var/lib/mysql/.tmp && rm -rf /var/lib/mysql/.tmp/* || mkdir -m 0750 /var/lib/mysql/.tmp) &&
            (test -d '/var/lib/mysql/#innodb_temp' && chmod 0750 '/var/lib/mysql/#innodb_temp' || true) &&
            touch /tmp/recovery-state/backup-fetched
            "
        - cwd: /home/mysql
        - require:
            - file: {{ mysql.defaults_file }}
            - test: walg-ready
            - file: /tmp/recovery-state
        - require_in:
            - mdb_mysql: mysql-init
        - unless:
            - ls /tmp/recovery-state/backup-fetched


mysql-restore:
  cmd.run:
    - name: |
        su mysql -c "
        /usr/local/yandex/mysql_restore.py apply-binlogs \
        --backup-name=\"{{ salt['pillar.get']('restore-from:backup-id', 'LATEST') }}\" \
        {% if salt['pillar.get']('restore-from:cid') %}
            {% if salt['pillar.get']('restore-from:time') %}
                --stop-datetime=\"{{ salt['pillar.get']('restore-from:time') }}\" \
                {% if (salt['pillar.get']('restore-from:until-binlog-last-modified-time')) and (environment in ('dev', 'qa')) %}
                   --until-binlog-last-modified-time=\"{{ salt['pillar.get']('restore-from:until-binlog-last-modified-time') }}\" \
                {% endif %}
            {% else %}
                --skip-pitr=True \
            {% endif %}
        {% endif %}
        --datadir=/var/lib/mysql \
        --defaults-file={{ mysql.defaults_file }} \
        --walg-config={{ mysql.walg_config }} \
        --max-allowed-packet=1073741824 \
        --port=3308 &&
        touch /tmp/recovery-state/prepare_mysql_on_high_port_done
        "
    - cwd: /home/mysql
    - require:
        - cmd: backup-fetched
        - file: {{ mysql.defaults_file }}
        - file: /usr/local/yandex/mysql_restore.py
        - file: /var/run/mysqld
    - require_in:
        - mdb_mysql: mysql-init
    - unless:
        - ls /tmp/recovery-state/prepare_mysql_on_high_port_done
{% endif %}
