{% from "components/mysql/map.jinja" import mysql with context %}
include:
    - components.mysql.configs.my_audit_cnf
{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
    - components.mysql.firewall
    - components.firewall.user_net_config
    - components.firewall.external_access_config
{% endif %}
{% if not salt['pillar.get']('skip-billing', False) %}
    - components.dbaas-billing.billing
{% endif %}
    - components.mysql.run-flush-hosts
    - components.mysql.mysync
    - components.mysql.service
    - components.mysql.walg-cron
{% if not mysql.is_replica %}
    - components.mysql.master-set-writable
    - components.mysql.users
    - components.mysql.users-grants

extend:
    mysql-users-req:
        test.nop:
            - require:
                - mdb_mysql: set-master-writable
    mysql-users-ready:
        test.nop:
            - require_in:
                - mdb_mysql: mysql-ensure-grants
    do-flush-hosts:
        mysql_query.run:
            - require:
                - mdb_mysql: mysql-ensure-grants

    mysql-ensure-grants:
        mdb_mysql.ensure_grants:
            - require_in:
                - cmd: mysql-wait-online
{% endif %}
