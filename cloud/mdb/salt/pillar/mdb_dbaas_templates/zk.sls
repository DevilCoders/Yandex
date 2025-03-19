{% set vtype = salt['grains.get']('test:vtype', 'porto') %}
{% set salt_version = salt['grains.get']('test:salt_version', '3002.7+ds-1+yandex0') %}
{% set salt_py_version = salt['grains.get']('test:salt_py_version', 3) %}
data:
    runlist:
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.zookeeper
        - components.dbaas-compute.scripts
        - components.cloud-init
{% endif %}
        - components.zk
{% if vtype != 'aws' %}
        - components.dbaas-billing
{% endif %}
    clickhouse:
        use_ssl: False
        cluster_name: template_cluster
        shard_id: template_shard
    zk:
        nodes:
            {{salt.grains.get('id')}}: 1
{% if salt_version %}
    salt_version: {{ salt_version }}
{% endif %}
{% if salt_py_version %}
    salt_py_version: {{ salt_py_version }}
{% endif %}
{% if vtype == 'porto' %}
    use_yasmagent: True
{% endif %}
    mdb_metrics:
        main:
            yasm_tags_cmd: '/usr/local/yasmagent/mdb_zk_getter.py'
    dbaas:
        cluster_id: template_cluster
