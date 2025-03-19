{%- set hostname = grains['nodename'] %}
{%- set base_role = grains['cluster_map']['hosts'][hostname]['base_role'] -%}
include:
  - common.push-client
  - nginx
  - s3-proxy
  - osquery
