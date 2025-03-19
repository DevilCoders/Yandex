{% set log_dir = '/var/log/dbaas-billing' %}

{% if salt['pillar.get']('data:billing:use_cloud_logbroker', False) %}
{%   set log_path = '{dir}/billing-yc.log'.format(dir=log_dir) %}
{% else %}
{%   set log_path = '{dir}/billing.log'.format(dir=log_dir) %}
{% endif %}

{% set service_log_path = '{dir}/service.log'.format(dir=log_dir) %}
{% set fqdn = salt['pillar.get']('data:dbaas:fqdn', salt['grains.get']('fqdn')) %}
{% set roles = [] %}
{% set checks = {} %}
{%
  set checks_map = {
      'components.postgresql_cluster': {
          'pg_ping': 120,
          'bouncer_ping': 120,
      },
      'components.clickhouse_cluster': {
          'ch_ping': 120,
      },
      'components.zk': {
          'zk_alive': 120,
      },
      'components.mongodb_cluster.mongod': {
          'mongodb_ping': 120,
      },
      'components.mongodb_cluster.mongocfg': {
          'mongodb_ping': 120,
      },
      'components.mongodb_cluster.mongos': {
          'mongos_ping': 120,
      },
      'components.mysql_cluster': {
          'mysql_ping': 120,
      },
      'components.redis_cluster': {
          'redis_alive': 120,
      },
      'components.elasticsearch_cluster.datanode': {
          'es_ping': 120,
      },
      'components.elasticsearch_cluster.masternode': {
          'es_ping': 120,
      },
      'components.kafka_cluster': {
          'kafka_ping': 120,
      },
  }
%}

{% for component in salt['pillar.get']('data:runlist') %}
{% do checks.update(checks_map.get(component, {})) %}
{% do roles.append(component.replace('components.', '')) %}
{% endfor %}
{% if salt['pillar.get']('data:billing:cluster_type') %}
{% do roles.append(salt['pillar.get']('data:billing:cluster_type')) %}
{% endif %}


dbaas-cron-restart:
    service.running:
        - name: dbaas-cron

/etc/dbaas-cron/conf.d/billing.conf:
    file.managed:
        - template: jinja
        - source: salt://components/dbaas-cron/conf/task_template.conf
        - mode: '0640'
        - user: root
        - group: monitor
        - context:
            module: billing
            args:
                log_file: {{ log_path }}
                service_log_file: {{ service_log_path }}
                rotate_size: 10485760
                checks: {{ checks }}
                params:
                    fqdn: {{ fqdn }}
                    roles: {{ roles }}
{% if salt['pillar.get']('data:dbaas') %}
                    resource_preset_id: {{ salt['pillar.get']('data:dbaas:flavor:name') }}
                    platform_id: {{ 'mdb-v' + salt['pillar.get']('data:dbaas:flavor:generation')|string }}
                    cores: {{ salt['pillar.get']('data:dbaas:flavor:cpu_limit')|int }}
                    core_fraction: {{ salt['pillar.get']('data:dbaas:flavor:cpu_fraction') }}
                    memory: {{ salt['pillar.get']('data:dbaas:flavor:memory_limit') }}
                    io_cores_limit: {{ salt['pillar.get']('data:dbaas:flavor:io_cores_limit') }}
                    cluster_type: {{ salt['pillar.get']('data:dbaas:cluster_type') }}
                    cluster_id: {{ salt['pillar.get']('data:dbaas:cluster_id') }}
                    cloud_id: {{ salt['pillar.get']('data:dbaas:cloud:cloud_ext_id') }}
                    folder_id: {{ salt['pillar.get']('data:dbaas:folder:folder_ext_id') }}
                    disk_size: {{ salt['pillar.get']('data:dbaas:space_limit') }}
                    disk_type_id: {{ salt['pillar.get']('data:dbaas:disk_type_id') }}
                    assign_public_ip: {{ 1 if salt['pillar.get']('data:dbaas:assign_public_ip') else 0 }}
                    compute_instance_id: {{ salt['pillar.get']('data:dbaas:vtype_id') }}
                    on_dedicated_host: {{ 1 if salt['pillar.get']('data:dbaas:on_dedicated_host') else 0 }}
{% else %}
                    cores: {{ salt['grains.get']('porto_resources:container:cpu_guarantee')[:-1] }}
                    core_fraction: 100
                    memory: {{ salt['grains.get']('porto_resources:container:memory_limit_total') }}
                    disk_size: {{ salt['grains.get']('porto_resources:volumes:/:space:limit') }}
                    disk_type_id: {{ salt['pillar.get']('data:billing:disk_type_id', 'local-ssd') }}
                    cluster_type: {{ salt['pillar.get']('data:billing:cluster_type') }}
                    cluster_id: {{ salt['grains.get']('porto_resources:container:hostname').split('.')[0][:-1] }}
                    cloud_id: {{ salt['pillar.get']('data:billing:cloud_id') }}
                    folder_id: {{ salt['pillar.get']('data:billing:folder_id') }}
                    resource_preset_id: "s3.custom"
                    platform_id: "mdb-v3"
                    io_cores_limit: 0
                    assign_public_ip: 0
                    compute_instance_id: None
                    on_dedicated_host: 0
{% endif %}
{% if salt['pillar.get']('data:elasticsearch') %}
                    edition: {{ salt['pillar.get']('data:elasticsearch:edition', 'basic') }}
{% elif salt['pillar.get']('data:mongodb') %}
                    edition: {{ salt.mdb_mongodb.version()['edition'] }}
{% endif %}
        - watch_in:
            - dbaas-cron-restart
