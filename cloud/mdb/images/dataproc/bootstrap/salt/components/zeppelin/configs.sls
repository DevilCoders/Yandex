{% set config_path = salt['pillar.get']('data:config:zeppelin_config_path', '/etc/zeppelin/conf') %}
{% set hive_config_path = salt['pillar.get']('data:config:hive_config_path', '/etc/hive/conf') %}

{% if salt['ydputils.is_masternode']() -%}

# Integrations with Hive
{% if 'hive' in salt['pillar.get']('data:services', []) %}
{{ config_path }}/hive-site.xml:
  file.symlink:
    - target: {{ hive_config_path }}/hive-site.xml
{% endif %}

{{ config_path }}/zeppelin-env.sh:
  file.append:
    - name: {{ config_path }}/zeppelin-env.sh
    - text:
      - ''
      - '# Yandex.Cloud Data Proc settings'
      - '# Use YARN cluster for application master'
      - 'export MASTER=yarn'
      - '# Use conda environment for pyspark tasks'
      - 'export PYSPARK_PYTHON="/opt/conda/bin/python"'
      - 'export PYSPARK_DRIVER_PYTHON="/opt/conda/bin/python"'
      - '# Add conda to env'
      - 'export PATH=$PATH:/opt/conda/bin'

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
