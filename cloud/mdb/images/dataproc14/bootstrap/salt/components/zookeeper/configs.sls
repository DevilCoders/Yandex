{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% set zk_config_path = salt['pillar.get']('data:config:zookeeper_config_path', '/etc/zookeeper/conf') %}
{% set zk_data_path = salt['pillar.get']('data:config:zookeeper_data_path', '/var/lib/zookeeper') %}

{{ zk_config_path }}/zoo.cfg:
    file.append:
        - name: {{ zk_config_path }}/zoo.cfg
        - text:
            - autopurge.purgeInterval=168

{% set counter = 0 %}
zk-hosts-is-present:
    {% set counter = counter + 1 %}
    file.append:
        - name: {{ zk_config_path }}/zoo.cfg
        - text:
        {%- for node in salt['ydputils.get_masternodes']() %}
            - server.{{ counter }}={{ node }}:2888:3888
        {%- endfor %}

zookeeper-config-myid:
    file.managed:
        - name: {{ zk_data_path }}/myid
        - contents:
            - {{ salt['grains.get']('dataproc:fqdn') }}
