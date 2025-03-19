{% from "components/postgres/pg.jinja" import pg with context %}

/etc/logrotate.d/postgresql-common:
    file.managed:
        - source: salt://components/postgres/conf/postgresql.logrotate
        - template: jinja
        - mode: 644
        - user: root
        - group: root

/usr/share/postgresql-common/pg_wrapper:
    file.managed:
        - source: salt://components/postgres/conf/pg_wrapper
        - mode: 755
        - user: root
        - group: root
        - makedirs: True
        - require:
            - pkg: postgresql-common-pkg

/lib/systemd/system/postgresql@.service:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/postgresql.service
        - require:
{%     if salt.pillar.get('data:database_slice:enable', False) and salt.dbaas.is_dataplane() %}
            - file: /etc/systemd/system/database.slice
{%     endif %}
            - pkg: postgresql-common-pkg
        - require_in:
            - service: postgresql-service
        - onchanges_in:
            - module: systemd-reload

/etc/security/limits.d/99-pgbouncer.conf:
    file.managed:
        - source: salt://components/postgres/conf/pgbouncer-limits.conf
        - mode: 644
        - user: root
        - group: root
        - makedirs: True

/etc/security/limits.d/80-postgres.conf:
    file.managed:
        - source: salt://components/postgres/conf/limits.conf
        - mode: 644
        - user: root
        - group: root
        - makedirs: True

/etc/sudoers.d/postgres:
    file.absent

hack_sudoers:
    file.append:
        - name: /etc/sudoers
        - text: '#includedir /etc/sudoers.d'

{{ pg.prefix }}/.bashrc:
    file.managed:
        - source: salt://components/postgres/conf/bashrc
        - template: jinja
        - user: postgres
        - group: postgres

{{ pg.prefix }}/.role.sh:
    file.managed:
        - source: salt://components/postgres/conf/role.sh
        - user: postgres
        - group: postgres
        - mode: 744

/etc/init.d/postgresql-pre:
    file.managed:
        - source: salt://components/postgres/conf/postgresql-pre.init.ubuntu
        - mode: 755
        - template: jinja
        - mode: 755
        - require:
            - file: {{ pg.data }}/conf.d/postgresql.conf
    cmd.wait:
        - name: update-rc.d postgresql-pre defaults 16 && /etc/init.d/postgresql-pre start
        - watch:
            - file: /etc/init.d/postgresql-pre
        - require_in:
            - service: postgresql-service

postgres-root-bashrc:
    file.accumulated:
        - name: root-bashrc
        - filename: /root/.bashrc
        - text: |-
            # Some useful things for postgres
            alias pg_top='su - postgres -c "pg_top -s1"'
{% if salt['pillar.get']('data:ship_logs', False) %}
            alias pglog='tail {{ pg.csv_log_file_path }}'
{% else %}
            alias pglog='tail {{ pg.log_file_path }}'
{% endif %}
            alias pg_log='pglog'
            alias dstatn='dstat --nocolor'
            psql () {
                sudo -u postgres psql "options='-c log_statement=none -c log_min_messages=panic -c log_min_error_statement=panic -c log_min_duration_statement=-1'" "${@:1}"
            }
            alias wal-g='wal-g --config /etc/wal-g/wal-g.yaml'
            export PATH="$PATH:{{ pg.bin_path }}"
        - require_in:
            - file: /root/.bashrc

{{ pg.prefix }}/.psqlrc:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/psqlrc
        - user: postgres
        - group: postgres
        - mode: 744

{{ pg.prefix }}/.psql_history-postgres:
    file.managed:
        - owner: postgres
        - group: postgres
        - mode: '0600'

{{ pg.prefix }}/.ssh/authorized_keys:
    file.copy:
        - source: /root/.ssh/authorized_keys2
        - user: postgres
        - group: postgres
        - mode: 600
        - require:
            - pkg: common-packages
            - file: {{ pg.prefix }}/.ssh
        - onchanges:
            - pkg: common-packages

/etc/rsyslog.conf:
    file.replace:
        - name: /etc/rsyslog.conf
        - pattern: '^local0\.\*'
        - repl: '#local0.*'
        - onlyif:
            - grep -E '^local0\.\*' /etc/ssh/sshd_config
    service.running:
        - name: rsyslog
        - restart: True
        - watch:
            - file: /etc/rsyslog.conf

/etc/barman.passwd:
    file.absent

/etc/rc.local:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/rc.local
        - user: root
        - group: root
        - mode: 755

/etc/pgswitch.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pgswitch.conf
        - mode: 600
        - require:
            - pkg: yamail-pgswitch

/etc/security/local.conf:
    file.managed:
        - source: salt://components/postgres/conf/security-local.conf
        - mode: 600
        - require:
            - pkg: common-packages

/usr/local/yandex/pg_wait_started.py:
    file.managed:
        - source: salt://components/postgres/conf/pg_wait_started.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/ensure_no_primary.py:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/ensure_no_primary.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/ensure_no_primary.sh:
    file.managed:
        - source: salt://components/postgres/conf/ensure_no_primary.sh
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/pg_wait_synced.py:
    file.managed:
        - source: salt://components/postgres/conf/pg_wait_synced.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/pg_stop.sh:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/pg_stop.sh
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/populate_recovery_conf.py:
    file.managed:
        - source: salt://components/postgres/conf/populate_recovery_conf.py
        - template: jinja
        - user: postgres
        - defaults:
            pg_prefix: {{ pg.data }}
        - group: postgres
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/pg_archive_wals.sh:
    file.absent

/etc/cron.yandex/clear_statements_stats.py:
    file.managed:
        - source: salt://components/postgres/conf/clear_statements_stats.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /etc/cron.yandex

{% if salt['grains.get']('virtual') == 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
/etc/cron.yandex/porto_attacher.py:
    file.absent

/etc/cron.d/barman_backup_to_porto:
    file.absent

{% endif %}

/etc/cron.d/pg_chown:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             @reboot root chown -R postgres.postgres /var/lib/postgresql >/dev/null 2>&1
        - mode: 644

/etc/cron.d/pg_porto_chown:
    file.absent

/etc/cron.yandex/pg_corruption_check.py:
    file.managed:
        - source: salt://components/postgres/conf/pg_corruption_check.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /etc/cron.yandex

/etc/cron.yandex/pg_amcheck.py:
    file.absent


{% set cron_jobs = [
                    'pg_auto_kill',
                    'pg_statements_stats',
                    'pg_rm_old_archive_status',
                    'odyssey_cores_cleanup',
                   ]
%}

{% for job in cron_jobs %}
/etc/cron.d/{{ job }}:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/cron.d/{{ job }}
        - mode: 644
{% endfor %}

/etc/logrotate.d/pg_auto_kill:
    file.managed:
        - source: salt://components/postgres/conf/pg_auto_kill.logrotate
        - mode: 644
        - user: root
        - group: root

/etc/cron.d/pg_renice_pgbouncer:
    file.absent

/etc/postgresql/{{ pg.version.major }}/data/pg_ident.conf:
    file.managed:
        - mode: 600
        - user: postgres
        - group: postgres
        - require:
            - file: /etc/postgresql/{{ pg.version.major }}/data
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - service: postgresql-service

/etc/postgresql/{{ pg.version.major }}/data/environment:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/environment
        - mode: 644
        - user: postgres
        - group: postgres
        - require:
            - file: /etc/postgresql/{{ pg.version.major }}/data
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - service: postgresql-service

/etc/postgresql/{{ pg.version.major }}/data/postgresql.conf:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/postgresql-include-only.conf
        - mode: 644
        - user: postgres
        - group: postgres
        - require:
            - file: /etc/postgresql/{{ pg.version.major }}/data
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - service: postgresql-service

/etc/postgresql/{{ pg.version.major }}/data/pg_ctl.conf:
    file.managed:
        - source: salt://components/postgres/conf/pg_ctl.conf
        - mode: 644
        - user: postgres
        - group: postgres
        - require:
            - file: /etc/postgresql/{{ pg.version.major }}/data
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - service: postgresql-service

/etc/postgresql/{{ pg.version.major }}/data/start.conf:
    file.managed:
        - source: salt://components/postgres/conf/start.conf
        - mode: 644
        - user: postgres
        - group: postgres
        - require:
            - file: /etc/postgresql/{{ pg.version.major }}/data
        - require_in:
            - file: {{ pg.data }}/conf.d/postgresql.conf
            - service: postgresql-service

include:
    - .pwfile
    - .pg_hba
    - .pgpass
    - .postgresql-conf
{% if pg.connection_pooler == 'pgbouncer' %}
    - .pgbouncer-userlist
    - .pgbouncer-ini
{% endif %}
    - components.common.systemd-reload
    - .pg_upgrade_cluster

extend:
    postgresql-service:
        service.running:
            - watch:
                - file: {{ pg.data }}/conf.d/postgresql.conf
                - file: {{ pg.data }}/conf.d/pg_hba.conf
                - file: /etc/postgresql/ssl/server.key
                - file: /etc/postgresql/ssl/server.crt
                - file: /etc/postgresql/ssl/allCAs.pem

{{ pg.data }}/conf.d/xlog_unpack.sh:
    file.absent

{{ pg.data }}/conf.d/barman-wal-restore.py:
    file.absent

{{ pg.prefix }}/.ssh/id_ecdsa:
    file.absent

/etc/cron.yandex/pg_recreate_unused_slot.py:
    file.managed:
        - source: salt://components/postgres/conf/pg_recreate_unused_slot.py
        - user: postgres
        - group: postgres
        - mode: 755
        - require:
            - file: /etc/cron.yandex

/etc/cron.d/pg_recreate_unused_slot:
    file.managed:
        - template: jinja
        - source: salt://components/postgres/conf/cron.d/pg_recreate_unused_slot
        - mode: 644
