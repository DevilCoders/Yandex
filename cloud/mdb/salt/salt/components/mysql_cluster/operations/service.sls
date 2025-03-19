{% from "components/mysql/map.jinja" import mysql with context %}

{% if salt['pillar.get']('pre_upgrade', False) or salt['pillar.get']('post_upgrade', False) %}

include:
    - components.mysql.configs.monitor_my_cnf
    - components.mysql.configs.mysql_my_cnf
    - components.mysql.mysync-config

{% else %}

include:
    - components.mysql.configs.my_cnf
    - components.mysql.cron
    - components.mysql.service
    - components.mysql.mysync
    - components.pushclient2.service
    - components.pushclient2.files_nop
    - components.dbaas-cron.service
{% if salt['pillar.get']('data:perf_diag:enabled', False) %}
    - components.mysql.perf-diag
{% else %}
    - components.mysql.perf-diag-disabled
{% endif %}
{% if salt['pillar.get']('service-restart') %}
    - components.mysql.restart
{% else %}
    - components.mysql.run-flush-hosts
{% endif %}
    - components.mysql.walg-config
{% if not mysql.is_replica %}
    - components.mysql.master-set-writable
{% else %}
    - components.mysql.replica-setup-replication
    - components.mysql.replica-set-online

extend:
    replica-set-online:
        cmd.run:
            - require:
                - mdb_mysql: mysql-setup-replication
{% endif %}

{% endif %}
