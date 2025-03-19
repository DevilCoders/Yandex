{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set zk_config_path = salt['pillar.get']('data:config:zookeeper_config_path', '/etc/zookeeper/conf') %}

{{ zk_config_path }}/zoo.cfg:
    file.append:
        - name: {{ zk_config_path }}/zoo.cfg
        - text:
            - autopurge.purgeInterval=168

zookeeper-config-myid:
    file.managed:
        - name: {{ zk_config_path }}/myid
        - contents:
            - {{ salt['grains.get']('dataproc:fqdn') }}
