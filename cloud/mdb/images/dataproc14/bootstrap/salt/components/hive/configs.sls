# Install Hadoop Configs
{% set hadoop_config_path = salt['pillar.get']('data:config:hadoop_config_path', '/etc/hadoop/conf') %}
{% set hive_config_path = salt['pillar.get']('data:config:hive_config_path', '/etc/hive/conf') %}
{% set services = salt['pillar.get']('data:services') %}

{% import 'components/hive/hive-site.sls' as c with context %}
{%- set hive = c.config_site['hive'] %}
{{ hive_config_path }}/hive-site.xml:
    hadoop-property.present:
        - config_path: {{ hive_config_path }}/hive-site.xml
        - properties:
            {{ hive | json }}

hive-patch-{{ hadoop_config_path }}/core-site.xml:
    hadoop-property.present:
        - config_path: {{ hadoop_config_path }}/core-site.xml
        - properties:
            'hadoop.proxyuser.hive.hosts': '*'
            'hadoop.proxyuser.hive.groups': '*'
        - require_in:
            - file: {{ hadoop_config_path }}/core-site.xml
{% if salt['ydputils.is_masternode']() %}
            - service: service-hadoop-yarn-resourcemanager
{% endif %}

hive-patch-hadoop-env.sh:
    file.append:
        - name: {{ hadoop_config_path }}/hadoop-env.sh
        - text:
            - HADOOP_CLASSPATH="${HADOOP_CLASSPATH}:/etc/hive/conf/"

{% set truststore = '/etc/ssl/certs/java/cacerts' %}
{% set trust_password = salt['pillar.get']('data:settings:truststore_password') %}

{{ hive_config_path }}/hive-env.sh:
    file.managed:
        - source: salt://{{ slspath }}/hive-env.jinja
        - template: jinja
        - defaults:
            truststore: {{ truststore }}
            trust_password: {{ trust_password }}
        - require:
            - pkg: hive_packages
{% if salt['ydputils.check_roles'](['masternode']) %}
        - require_in:
            - service: service-hive-server2
{% endif %}

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
        - require:
            - pkg: spark_packages
{%- endif %}
