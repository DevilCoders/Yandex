# Install Hadoop Configs
{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set hive_config_path = salt['pillar.get']('data:config:hive_config_path', '/etc/hive/conf') %}
{% set services = salt['pillar.get']('data:services') %}
{% set masternode = salt['ydputils.get_masternodes']()[0] %}
{% set warehouse = 'hdfs://' + masternode + ':8020/user/hive/warehouse'%}

{% import 'components/hive/hive-site.sls' as c with context %}
{%- set hive = c.config_site['hive'] %}
{{ hive_config_path }}/hive-site.xml:
    hadoop-property.present:
        - config_path: {{ hive_config_path }}/hive-site.xml
        - properties:
            {{ hive | json }}

{% import 'components/hive/hivemetastore-site.sls' as hivemetastore with context %}
{%- set metastore = hivemetastore.config_site['hivemetastore'] %}
{{ hive_config_path }}/hivemetastore-site.xml:
    hadoop-property.present:
        - config_path: {{ hive_config_path }}/hivemetastore-site.xml
        - properties:
            {{ metastore | json }}

{% import 'components/hive/hiveserver2-site.sls' as hiveserver with context %}
{%- set server = hiveserver.config_site['hiveserver2'] %}
{{ hive_config_path }}/hiveserver2-site.xml:
    hadoop-property.present:
        - config_path: {{ hive_config_path }}/hiveserver2-site.xml
        - properties:
            {{ server | json }}

hive-patch-{{ hadoop_config_path }}/core-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/core-site.xml
        - properties:
            'hadoop.proxyuser.hive.hosts': '*'
            'hadoop.proxyuser.hive.groups': '*'
        - require_in:
            - hadoop-property: {{ hadoop_config_path }}/core-site.xml
{% if salt['ydputils.is_masternode']() %}
            - service: service-hadoop-yarn-resourcemanager
{% endif %}

hive-patch-hadoop-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/hadoop-env.sh
        - text:
            - HADOOP_CLASSPATH="${HADOOP_CLASSPATH}:/etc/hive/conf/"

{% if salt['ydputils.check_roles'](['masternode']) and 'spark' in services -%}
{%- set spark_config_path = salt['pillar.get']('data:config:spark_config_path', '/etc/spark/conf') %}
{{ spark_config_path }}/hive-site.xml:
    hadoop-property.present:
        - config_path: {{ spark_config_path }}/hive-site.xml
        - properties:
            'hive.metastore.uris': {{ hive['hive.metastore.uris'] }}
            'javax.jdo.option.ConnectionUserName': {{ hive['javax.jdo.option.ConnectionUserName'] }}
            'javax.jdo.option.ConnectionPassword': {{ hive['javax.jdo.option.ConnectionPassword'] }}
            'hive.metastore.connect.retries': 15
            'hive.metastore.warehouse.dir': {{ warehouse }}
        - require:
            - pkg: spark_packages
{%- endif %}
