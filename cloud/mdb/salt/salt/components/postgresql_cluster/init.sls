{% set environment = salt['pillar.get']('yandex:environment', 'dev') %}

include:
{% if salt['pillar.get']('data:dbaas:vtype') == 'compute' %}
    - components.dbaas-compute
    - components.dbaas-compute.network
    - components.dbaas-compute.apparmor.postgresql
    - components.firewall
    - components.firewall.external_access_config
    - components.oslogin
{% elif salt['pillar.get']('data:dbaas:vtype') == 'porto' %}
    - components.dbaas-porto
    - components.firewall
{% endif %}
    - components.dbaas-billing
    - components.monrun2.mdb-dns-resolve-checker
    - components.dbaas-cron.tasks.mdbdns_postgres
    - components.postgres
    - components.pg-dbs.unmanaged
