include:
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.dbaas-compute
    - components.dbaas-compute.network
    - components.dbaas-compute.apparmor.elasticsearch
    - components.firewall
    - components.firewall.external_access_config
    - components.oslogin
    - .oslogin
{% elif salt.pillar.get('data:dbaas:vtype') == 'porto' %}
    - components.dbaas-porto
{% endif %}
    - components.dbaas-billing
    - components.dbaas-cron.tasks.mdbdns_elasticsearch
    - components.monrun2.mdb-dns-resolve-checker
    - components.opensearch
