{%- set hostname = grains['nodename'] -%}
{%- set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}

include:
  - .common
{% if 'cloudgate' in host_roles %}
  - .cloudgate
{% elif 'slb-adapter' in host_roles %}
  - .slb_adapter
{% else %}
  - .head
  {% if 'compute' in host_roles %}
  - .compute
  {% endif %}
{% endif %}
