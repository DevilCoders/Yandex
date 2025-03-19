{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.redis
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.dbaas-billing
        - components.redis
    mdb_metrics:
        enabled: True
    dbaas:
        flavor:
            cpu_limit: 1
            memory_limit: 4294967296
    database_slice:
        enable: True
    use_yasmagent: False
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3
    allow_salt_version_update: True
redis-master:
    {{ salt['grains.get']('id') }}
