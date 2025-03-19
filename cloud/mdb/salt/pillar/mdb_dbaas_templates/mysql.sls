{% from "map.jinja" import mysql_versions_package_map with context %}
{% from "mdb_dbaas_templates/init.sls" import env with context %}
{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.mysql
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.dbaas-billing
        - components.mysql
    mysql:
        use_ssl: False
    start_mysync: False
{% set mysql_major_version = '5.7' %}
    versions:
        mysql:
            major_version: {{ mysql_major_version | tojson }}
            minor_version: {{ mysql_versions_package_map[(env, mysql_major_version)]['minor_version'] | tojson }}
            package_version: {{ mysql_versions_package_map[(env, mysql_major_version)]['package_version'] }}

include:
    - porto.prod.mysql.users.dev.common
