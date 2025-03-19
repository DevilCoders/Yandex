include:
{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
    - components.dbaas-compute
    - components.dbaas-compute.network
    - components.firewall
    - components.firewall.external_access_config
    - components.dbaas-compute.apparmor.redis
    - components.redis.firewall
    - components.oslogin
    - .oslogin
{% elif salt.pillar.get('data:dbaas:vtype') == 'porto' %}
    - components.dbaas-porto
    - components.firewall
{% endif %}
    - components.dbaas-billing
    - components.redis
    - components.dbaas-cron.tasks.mdbdns_redis
    - components.monrun2.mdb-dns-resolve-checker
