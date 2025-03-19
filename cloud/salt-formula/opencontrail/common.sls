{%- from 'opencontrail/map.jinja' import current_oct_cluster -%}
{%- set hostname = grains['nodename'] -%}
{%- set ipv4 = grains['cluster_map']['hosts'][hostname]['ipv4']['addr'] -%}

{% set include_list = [] %}

{# In the first phase of bootstrap process we do not have 'oct_conf'
   section, because service VMs are not started yet. 'oct_conf' grain
   is used in nginx templates in opencontrail.balancer_current_cluster. #}
{% if ipv4 != '127.0.0.1' and (current_oct_cluster.get('roles', {}).get('oct_conf') or current_oct_cluster.get('roles', {}).get('oct_head')) %}
  {% do include_list.append('opencontrail.balancer_current_cluster') %}
{% endif %}

{% do include_list.append('common.network_interfaces.base_ipv4') %}

{% if include_list %}
include:
  {% for item in include_list %}
- {{ item }}
  {% endfor %}
{% endif %}

opencontrail_common_packages:
  yc_pkg.installed:
    - pkgs:
      - python-anyjson
      - contrail-utils
      - contrail-nodemgr
      - python-contrail
      - contrail-lib
      - python-kombu
      - contrail-api-client
      - yc-network-oncall-tools

{%- for file in ['introspect_mon','discovery_mon'] %}
/usr/local/bin/{{ file }}:
  file.managed:
    - source: salt://{{ slspath }}/files/{{ file }}
    - mode: 755
{% endfor -%}

/usr/local/lib/python2.7/dist-packages/introspect.py:
  file.managed:
    - source: salt://{{ slspath }}/files/mon/introspect.py
    - mode: 644

{%- from slspath + "/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
