{%- set hostname = grains['nodename'] -%}
{%- set environment = grains['cluster_map']['environment'] -%}
{%- set roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}

base:
  '*':
    - dns
    - common
    - mine
    - resources
    - release
    - roles
{% if 'head' in roles or 'seed' in roles %}
    - secrets.salt
{% endif  %}
    - ignore_missing: True
  'cluster_map:stand_type:virtual':
    - match: grain
    - common.cloudvm
    - test_config
  'cluster_map:stand_type:hardware':
    - match: grain
    - common.{{ environment }}
