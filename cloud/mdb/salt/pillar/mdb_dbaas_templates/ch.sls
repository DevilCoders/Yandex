{% set hostname = salt.grains.get('id') %}
{% set vtype    = salt['grains.get']('test:vtype', 'porto') %}
data:
    runlist:
        - components.clickhouse
{% if vtype == 'compute' %}
        - components.firewall
        - components.dbaas-compute.apparmor
        - components.dbaas-compute.apparmor.clickhouse
        - components.dbaas-compute.scripts
        - components.dbaas-compute.network
        - components.cloud-init
        - components.linux-kernel
{% endif %}
{% if vtype == 'porto' %}
        - components.dbaas-porto
{% endif %}
{% if vtype != 'aws' %}
        - components.dbaas-billing
{% endif %}
    dbaas:
        cluster_type: clickhouse_cluster
        shard_id: template_shard_id
        shard_name: template_shard_name
        geo: template_az
        flavor:
            memory_guarantee: 2147483648
        cluster:
            subclusters:
                subcluster_id:
                   roles: ['clickhouse_cluster']
                   shards:
                       template_shard_id:
                           name: template_shard_name
                           hosts:
                               {{ hostname }}:
                                   geo: template_az
    clickhouse:
        cluster_name: template_cluster
        users: {}
        use_ssl: False
        periodic_backups: False
        initialize_s3: False
    ch_backup:
        encryption_key: template_encryption_key
{% if vtype == 'porto' %}
    use_yasmagent: True
{% endif %}
{% if vtype == 'aws' %}
    use_mdbsecrets: False
{% endif %}
    salt_version: 3002.7+ds-1+yandex0
    salt_py_version: 3
    allow_salt_version_update: True

restart-check: False

include:
{% if vtype == 'compute' %}
    - generated.compute.ch_template_version
{% else %}
    - generated.porto.ch_template_version
{% endif %}
