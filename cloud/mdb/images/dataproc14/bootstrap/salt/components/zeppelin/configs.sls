{% set config_path = salt['pillar.get']('data:config:zeppelin_config_path', '/etc/zeppelin/conf') %}
{% set hive_config_path = salt['pillar.get']('data:config:hive_config_path', '/etc/hive/conf') %}

{% if salt['ydputils.is_masternode']() -%}

# Integrations with Hive
    {% if 'hive' in salt['pillar.get']('data:services', []) %}
{{ config_path }}/hive-site.xml:
    file.symlink:
        - target: {{ hive_config_path }}/hive-site.xml
    {% else %}
zeppelin-site-disable-useHiveContext:
    hadoop-property.present:
        - config_path: {{ config_path }}/zeppelin-site.xml
        - require:
            - {{ config_path }}/zeppelin-site.xml
        - properties:
            'zeppelin.spark.useHiveContext': 'false'
    {% endif %}

{{ config_path }}/zeppelin-env.sh:
    file.managed:
        - name: {{ config_path }}/zeppelin-env.sh
        - source: salt://{{ slspath }}/conf/zeppelin-env.sh

{% import 'components/zeppelin/zeppelin-site.sls' as c with context %}

{{ config_path }}/zeppelin-site.xml:
    hadoop-property.present:
        - config_path: {{ config_path }}/zeppelin-site.xml
        - properties:
            {{ c.config_site['zeppelin'] | json }}

{{ config_path }}/interpreter.json:
    file.managed:
        - name: {{ config_path }}/interpreter.json
        - template: jinja
        - user: zeppelin
        - group: zeppelin
        - source: salt://{{ slspath }}/conf/interpreter.json
{%- endif %}
