{% set environment = salt['pillar.get']('yandex:environment', 'dev') %}

include:
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.dbaas-compute
    - components.dbaas-compute.network
    - components.dbaas-compute.apparmor.greenplum
    - components.firewall
    - components.firewall.external_access_config
    - components.oslogin
    - components.greenplum_cluster.oslogin
{% elif salt.pillar.get('data:dbaas:vtype') == 'porto' %}
    - components.dbaas-porto
    - components.firewall
{% endif %}
    - components.greenplum
