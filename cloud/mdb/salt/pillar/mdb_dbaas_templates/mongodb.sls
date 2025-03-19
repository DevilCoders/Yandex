{% from "map.jinja" import mongodb_versions_package_map with context %}
{% from "mdb_dbaas_templates/init.sls" import env with context %}
{% set vtype = salt['grains.get']('test:vtype', 'porto') %}

{% set mongodb_major_version = '4.4' %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.mongodb
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
        - components.dbaas-billing
        - components.mongodb

    versions:
      mongodb:
        major_version: {{ mongodb_major_version | tojson }}
        minor_version: {{ mongodb_versions_package_map[(env, mongodb_major_version, 'default')]['minor_version'] | tojson }}
        package_version: {{ mongodb_versions_package_map[(env, mongodb_major_version, 'default')]['package_version'] | tojson }}
    mongodb:
        use_mongod: True
        databases:
            dummy: {}
        feature_compatibility_version: {{ mongodb_major_version | tojson }}
    dbaas:
        shard_hosts:
            - {{ salt['grains.get']('id') }}
        flavor:
            cpu_limit: 1
            memory_guarantee: 4294967296
            memory_limit: 4294967296
        space_limit: 10737418240
    walg:
        enabled: False
        install: True
    database_slice:
        enable: True
    mdb_metrics:
        enabled: True
    mdb_mongo_tools:
        enabled: False
        install: True
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3
    allow_salt_version_update: True
{% if vtype != 'compute' %}
    use_yasmagent: True
{% endif %}

include:
    - porto.prod.mongodb.dev.common
