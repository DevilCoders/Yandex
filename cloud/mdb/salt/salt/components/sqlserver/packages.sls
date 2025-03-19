{% from "components/sqlserver/map.jinja" import sqlserver with context %}

include:
    - components.mdb-telegraf-windows.packages

walg-package:
    mdb_windows.nupkg_installed:
        - name: wal-g-sqlserver
        - version: '1276.84820019.0'
        - stop_service: walg-proxy
        - require:
            - cmd: mdb-repo

{% if sqlserver.edition == 'standard' %}
{% if salt['mdb_windows.get_hosts_by_role']('sqlserver_cluster')|length == 2 %}
mssync-package:
    mdb_windows.nupkg_installed:
        - name: MSSync
        - version: '1.9349767.0'
        - require:
            - cmd: mdb-repo
{% endif %}
{% endif %}
