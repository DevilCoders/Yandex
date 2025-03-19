{% from "components/mysql/map.jinja" import mysql with context %}
include:
  - .configs
  - .service
{% if salt['pillar.get']('data:mysql:use_ssl', True) %}
  - .ssl
{% endif %}
{% if mysql.is_replica %}
  - .replica
{% else %}
  - .master
  - .databases
  - .users
  - .users-grants
  - .timezones
{% endif %}
  - .sqls
  - .mysync
  - .mysync-install
  - .mysql-shell-install
  - components.logrotate
  - components.monrun2.mysql
  - .yasmagent
  - .cron
  - components.yasmagent
{% if salt['pillar.get']('data:mdb_metrics:enabled', True) %}
  - .mdb-metrics
{% else %}
  - components.mdb-metrics.disable
{% endif %}
{% if salt['pillar.get']('data:ship_logs', False) %}
  - components.pushclient2
  - .pushclient
{% if salt['pillar.get']('data:perf_diag:enabled', False) %}
  - .perf-diag
{% else %}
  - .perf-diag-disabled
{% endif %}
{% endif %}
{% if salt['pillar.get']('data:use_walg', True) %}
  - .walg
{% endif %}
{% if salt['pillar.get']('service-restart', False) or salt['pillar.get']('upgrade') %}
  - .restart
{% endif %}
{% if salt['pillar.get']('data:dbaas:cluster') %}
  - .resize
{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
  - components.firewall
  - components.firewall.external_access_config
  - .firewall
{% endif %}
{% if salt['pillar.get']('data:dbaas:vtype') == 'porto' %}
  - components.firewall
  - .firewall
{% endif %}
{% endif %}

extend:
{% if not mysql.is_replica %}
    mysql-users-req:
        test.nop:
            - require:
                - test: mysql-initialized
                - mdb_mysql: set-master-writable

    mysql-databases-req:
        test.nop:
            - require:
                - test: mysql-initialized

    mysql-users-ready:
        test.nop:
            - require_in:
                - test: mysql-ready
                - mdb_mysql: mysql-ensure-grants

    mysql-databases-ready:
        test.nop:
            - require_in:
                - test: mysql-ready
                - mdb_mysql: mysql-ensure-grants

    mysql-wait-online:
        cmd.run:
            - require:
                - mdb_mysql: mysql-ensure-grants
{% endif %}

{% if salt['pillar.get']('data:start_mysync', True) %}
    mysync-ha-configure:
        cmd.run:
            - require_in:
                - mdb_mysql: mysql-init
{% endif %}

{% if salt['pillar.get']('data:ship_logs', False) %}
    perf-diag-req:
        test.nop:
            - require:
                - pkg: dbaas-cron
                - pkg: pushclient
{% endif %}

    mysql-settings-req:
        test.nop:
            - require:
                - test: mysql-initialized

    mysync-pre-install:
        test.nop:
            - require:
                - user: mysql-user

    /etc/mysync.yaml:
        file.managed:
            - require:
                - test: mysync-installed

    mysync-pkg:
        pkg.installed:
            - watch_in:
                - service: mysync

{% if salt[ 'pillar.get' ]('data:start_mysync', True) %}
    mysql-service:
        service.running:
            - require:
                - service: mysync
{% endif %}

python-mysqldb:
  pkg.installed:
    - require_in:
        - test: mysql-service-req

python3-mysqldb:
  pkg.installed:
    - require_in:
        - test: mysql-service-req

mysql-group:
  group.present:
    - name: mysql
    - system: True

mysql-user:
  user.present:
    - name: mysql
    - gid: mysql
    - system: True
    - require:
      - group: mysql-group

/etc/mysql/init.sql:
  file.managed:
    - user: mysql
    - group: mysql
    - dir_mode: 755
    - mode: 400
    - makedirs: True
    - template: jinja
    - source: salt://{{ slspath }}/conf/init.sql
    - require:
        - user: mysql-user
        - pkg: mysql

/usr/local/yandex/mysql_init_timezones.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_init_timezones.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/mysql_upgrade.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_upgrade.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/mysql_flush_logs.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_flush_logs.sh
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/mysql_mdb_repl_mon.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_mdb_repl_mon.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

restart-mysql_mdb_repl_mon.py:
    cmd.run:
        - name: "killall -9 mysql_mdb_repl_mon.py ; echo \"ignore return code '$?'. success_retcodes not supported\""
        - onchanges:
            - file: /usr/local/yandex/mysql_mdb_repl_mon.py

/usr/local/yandex/ensure_no_primary.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_no_primary.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/ensure_no_primary.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/ensure_no_primary.sh
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/usr/local/yandex/mysql_innodb_status_output.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_innodb_status_output.py
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: /usr/local/yandex

/var/lib/mysql:
  file.directory:
    - user: mysql
    - group: mysql
    - dir_mode: 750
    - require:
      - user: mysql-user

/var/lib/mysql/.tmp:
  file.directory:
    - user: mysql
    - group: mysql
    - dir_mode: 750
    - require:
      - file: /var/lib/mysql

/var/run/mysqld:
  file.directory:
    - user: mysql
    - group: mysql
    - dir_mode: 775
    - require:
      - user: mysql-user

mysql:
  pkg.installed:
    - pkgs:
{% if mysql.version.num >= 800 %}
      - percona-server-server: {{ mysql.version.pkg }}
      - percona-server-common: {{ mysql.version.pkg }}
      - percona-server-client: {{ mysql.version.pkg }}
{% else %}
      {#- TODO: remove after complete upgrade from 5.7.25 #}
      {%- set pkg_sub_name = 'xtradb-cluster' if mysql.version.minor == '5.7.25' else 'server' %}
      - percona-{{ pkg_sub_name }}-server-5.7: {{ mysql.version.pkg }}
      - percona-{{ pkg_sub_name }}-common-5.7: {{ mysql.version.pkg }}
      - percona-{{ pkg_sub_name }}-client-5.7: {{ mysql.version.pkg }}
{% endif %}
    - prereq_in:
        - cmd: repositories-ready
    - require_in:
        - test: mysql-service-req

{# TODO: remove after complete upgrade from 5.7.25 #}
{% if mysql.version.num < 800 and mysql.version.minor != '5.7.25' %}
xtradb-cluster:
  pkg.purged:
    - pkgs:
      - percona-xtradb-cluster-server-5.7
      - percona-xtradb-cluster-common-5.7
      - percona-xtradb-cluster-client-5.7
    - require_in:
        - pkg: mysql
{% endif %}

xtrabackup:
  pkg.installed:
    - pkgs:
{% if mysql.version.num >= 800 %}
      - percona-xtrabackup-80: {{ mysql.xtrabackup_version.pkg }}
{% else %}
      - percona-xtrabackup-24: {{ mysql.xtrabackup_version.pkg }}
{% endif %}
    - prereq_in:
        - cmd: repositories-ready
    - require_in:
        - test: mysql-service-req

mdb-my-toolkit:
  pkg.installed:
    - version: '1.9317372'
    - prereq_in:
        - cmd: repositories-ready
    - require_in:
        - test: mysql-service-req

libjemalloc1:
  pkg.installed:
    - version: 3.6.0-11
    - require_in:
        - test: mysql-service-req

/etc/init.d/mysql:
    file.absent

/etc/systemd/system/mysql.service:
    file.absent

/lib/systemd/system/mysql.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mysql.service
        - mode: 644
        - require:
            - pkg: mysql
        - require_in:
            - test: mysql-service-req
        - onchanges_in:
            - module: systemd-reload

/var/log/mysql:
  file.directory:
    - user: mysql
    - group: mysql
    - dir_mode: 755
    - require:
      - pkg: mysql
    - require_in:
        - test: mysql-service-req

# datadir ready, daemon started, binlogs applied
mysql-initialized:
  test.nop:
    - require:
      - test: mysql-service-ready
      - file: /root/.my.cnf
      - file: /home/mysql/.my.cnf
      - file: /home/monitor/.my.cnf

# all user are created with new passwords
mysql-ready:
  test.nop:
    - require:
      - test: mysql-initialized
