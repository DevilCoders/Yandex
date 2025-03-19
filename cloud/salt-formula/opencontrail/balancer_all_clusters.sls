{%- from 'opencontrail/map.jinja' import all_oct_clusters -%}

include:
  - nginx

{% for oct_cluster_id, oct_cluster in all_oct_clusters | dictsort %}
oct-{{ oct_cluster_id }}-balancer:
  host.present:
    - ip: 127.0.0.{{ loop.index0 + 101 }}
{% endfor %}

{%- set nginx_configs = ['contrail-api-all-clusters.conf'] %}
{%- include 'nginx/install_configs.sls' %}
