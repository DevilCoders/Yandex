{%- set fqdn = grains['nodename'] %}
{%- set environment = grains['cluster_map']['environment'] %}
{%- set base_role = salt["grains.get"]("cluster_map:hosts:%s:base_role" % fqdn) %}
{%- set roles = salt["grains.get"]("cluster_map:hosts:%s:roles" % fqdn) %}
{%- set osquery_tags = salt['pillar.get']('osquery_tags', {}) %}
{%- set osquery_tag = 'ycloud-svc' %}

{%- if base_role in osquery_tags %}
  {%- set osquery_tag = osquery_tags[base_role] %}
{%- elif base_role.endswith('-dn') and 'kikimr' in roles %}
  {%- set osquery_tag = osquery_tags['kikimr_dyn_nodes'] %}
{%- endif %}


{%- if osquery_tag %}

/etc/osquery.tag:
  file.managed:
    - contents: {{ osquery_tag }}

osquery:
  yc_pkg.installed:
    - pkgs:
      - osquery-vanilla
      - osquery-yandex-generic-config
    - require:
      - file: /etc/osquery.tag

{%- endif %}
