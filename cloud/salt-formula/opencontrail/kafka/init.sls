{%- from 'opencontrail/map.jinja' import oct_db_servers, oct_collect_servers -%}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] %}
{%- set database_id = (oct_collect_servers|sort).index(hostname) + 1 %}

/usr/share/kafka/config/server.properties:
  file.managed:
    - source: salt://{{ slspath }}/files/kafka/server.properties
    - template: jinja
    - makedirs: True
    - defaults:
        database_id: {{ database_id }}
        oct_collect_servers: {{ oct_collect_servers }}
        oct_db_servers: {{ oct_db_servers }}

opencontrail_kafka_packages:
  yc_pkg.installed:
    - pkgs:
      - kafka
    - hold_pinned_pkgs: True
    - reload_modules: True
    - require:
      - file: /usr/share/kafka/config/server.properties

# Overwriting package files in /usr/share is bad, but at least they ain't marked conffiles
kafka:
  service.running:
    - enable: True
    - require:
      - yc_pkg: opencontrail_kafka_packages
      - file: /usr/share/kafka/config/server.properties
    - watch:
      - file: /usr/share/kafka/config/server.properties

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
