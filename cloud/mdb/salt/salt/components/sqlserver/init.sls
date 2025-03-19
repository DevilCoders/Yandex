{% from "components/sqlserver/map.jinja" import sqlserver with context %}

include:
    - .packages
{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
    - .walg
    - .install
    - .service
    - .ssl
    - .logins
    - .telegraf
{% if not sqlserver.is_replica %}
    - .master
    - .master_databases
    - .users
{% else %}
    - .replica
    - .replica_databases
{% endif %}
{% if salt['pillar.get']('service-restart', False) %}
    - .restart
{% endif %}
{% if sqlserver.edition == 'standard' %}
{% if salt['mdb_windows.get_hosts_by_role']('sqlserver_cluster')|length == 2 %}
    - .mssync
{% endif %}
{% endif %}
    - .firewall
    - .sp_configure
    - .sql
    - .ag_config
    - .tasks
    - .ps

extend:

    sqlserver-logins-req:
        test.nop:
            - require:
                - test: sqlserver-service-ready

    sqlserver-ag-req:
        test.nop:
            - require:
                - test: windows-cluster-ready
                - test: sqlserver-service-ready
                - test: sqlserver-agconfig-ready

    sqlserver-databases-req:
        test.nop:
            - require:
                - test: walg-ready
                - test: sqlserver-service-ready
                - test: sqlserver-agconfig-ready

    sqlserver-tasks-req:
        test.nop:
            - require:
                - test: sqlserver-databases-ready
                - file: 'C:\Program Files\Mdb'

{% if not sqlserver.is_replica %}

    sqlserver-users-req:
        test.nop:
            - require:
                - test: sqlserver-service-ready
                - test: sqlserver-logins-ready
                - test: sqlserver-databases-ready
                - test: sqlserver-config-ready
{% endif %}

    sqlserver-config-req:
        test.nop:
            - require:
                - test: sqlserver-service-ready
{% endif %}
