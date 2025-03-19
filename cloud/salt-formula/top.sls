{%- set roles = grains['cluster_map']['hosts'][grains['nodename']]['roles'] %}

base:
  '*':
  {%- for role in roles %}
    - roles.{{ role }}
  {%- endfor %}
